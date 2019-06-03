#include "TcpConnection.h"

#include <unistd.h> // read

#include <iostream>

#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

using namespace tinyWS;
using namespace std::placeholders;

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InternetAddress &localAddress,
                             const InternetAddress &peerAddress)
                             : loop_(loop),
                               name_(name),
                               state_(kConnecting),
                               socket_(new Socket(sockfd)),
                               channel_(new Channel(loop, sockfd)),
                               localAddress_(localAddress),
                               peerAddress_(peerAddress) {
    // 设置回调函数
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
    socket_->setKeepAlive(true);
}

void TcpConnection::send(const std::string &message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::send(const void *message, size_t len) {
    const char *cStr = static_cast<const char*>(message);
    std::string str(cStr, len);
    send(str);
}

void TcpConnection::shutdown() {
    // FIXME: use compare and swap
    if (state_ == kConnected) {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::setKeepAlive(bool on) {
    socket_->setKeepAlive(on);
}

TcpConnection::~TcpConnection() {
    std::cout << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd() << std::endl;

    ::close(channel_->fd());
}

void TcpConnection::setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
}

void TcpConnection::setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
}

void TcpConnection::setCloseCallback(const CloseCallback &cb) {
    closeCallback_ = cb;
}

void TcpConnection::setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
}

void TcpConnection::setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark) {
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
}

void TcpConnection::connectionEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectionDestoryed() {
    loop_->assertInLoopThread();
    assert(state_ == kConnected);
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
    loop_->removeChannel(channel_.get());
}

std::string TcpConnection::name() {
    return name_;
}

EventLoop* TcpConnection::getLoop() const {
    return loop_;
}

const InternetAddress& TcpConnection::localAddress() const {
    return localAddress_;
}

const InternetAddress& TcpConnection::peerAddress() const {
    return peerAddress_;
}

bool TcpConnection::connected() const {
    return state_ == kConnected;
}

bool TcpConnection::disconnected() const {
    return state_ == kDisconnected;
}

void TcpConnection::setState(tinyWS::TcpConnection::StateE s) {
    state_ = s;
}

void TcpConnection::handleRead(Timer::TimeType receiveTime) {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(socket_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        std::cout << "TcpConnection::handleRead" << std::endl;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = ::write(
                socket_->fd(),
                outputBuffer_.peek(),
                outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(static_cast<size_t>(n));
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    // 该函数作为 Channel 的写回调函数，在 IO 线程中执行
                    shutdownInLoop();
                }
            } else {
                std::cout << "I am going to write more data" << std::endl;
            }
        } else {
            std::cout << "TcpConnection::handleWrite" << std:: endl;
        }
    } else {
        std::cout << "Connection is down, no more writing" << std::endl;
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    std::cout << "TcpConnection::handleClose state = " << state_ << std::endl;
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    channel_->disableAll();
    // must be the last line
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError() {
    int err = socket_->getSocketError();
    std::cout << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << std::endl;
}


void TcpConnection::sendInLoop(const std::string &message) {
    loop_->assertInLoopThread();
    ssize_t n = 0;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        n = ::write(socket_->fd(), message.data(), message.size());
        if (n >= 0) {
            if (static_cast<size_t>(n) < message.size()) {
                std::cout << "I am going to write more data" << std::endl;
            } else {
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
        } else {
            n = 0;
            if (errno != EWOULDBLOCK) {
                std::cout << "TcpConnection::sendInLoop" << std::endl;
            }
        }

        assert(n >= 0);
        if (static_cast<size_t>(n) < message.size()) {
            outputBuffer_.append(message.data() + n, message.size() - n);
            if (!channel_->isWriting()) {
                channel_->enableWriting();
            }
        }
    }
}

//void TcpConnection::sendInLoop(const void *message, size_t len) {
//
//}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}