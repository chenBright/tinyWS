#include "TcpConnection.h"

#include <unistd.h> // read

#include <iostream>

#include "../base/Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

using namespace tinyWS;
using namespace std::placeholders;

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             Socket socket,
                             const InternetAddress &localAddress,
                             const InternetAddress &peerAddress)
                             : loop_(loop),
                               name_(name),
                               state_(kConnecting),
                               socket_(new Socket(std::move(socket))),
                               channel_(new Channel(loop, socket_->fd())),
                               localAddress_(localAddress),
                               peerAddress_(peerAddress) {
    debug() << "move fd = " << socket_->fd() << std::endl;
    // 设置回调函数
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));

    debug() << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << socket_->fd();

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    debug() << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd()
            << " state=" << state_;
    // TODO stateToString()：将状态转成字符串信息

    assert(state_ == kDisconnected);
}

void TcpConnection::send(const std::string &message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            // 如果当前线程为 IO 线程，则直接发送。
            sendInLoop(message);
        } else {
            // 如果当前线程不是 IO 线程，则将发送数据工作转移到 IO 线程。
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
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::setContext(const HttpContext &context) {
    context_ = context;
}

const HttpContext& TcpConnection::getContext() const {
    return context_;
}

HttpContext* TcpConnection::getMutableContext() {
    return &context_;
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::setKeepAlive(bool on) {
    socket_->setKeepAlive(on);
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
    // 将 TcpConnection 绑定到 channel 上
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectionDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    // 将 Channel 从 Epoll 中移除
    channel_->remove();
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
        debug(LogLevel::ERROR) << "TcpConnection::handleRead" << std::endl;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        // 如果 Channel 可写，则直接发送数据。
        ssize_t n = ::write(
                socket_->fd(),
                outputBuffer_.peek(),
                outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(static_cast<size_t>(n)); // 更新 outputBuffer_ 中的缓冲区索引
            if (outputBuffer_.readableBytes() == 0) {
                // 如果 outputBuffer_ 中没有可读数据，即数据已经发送完毕，
                // 则立即设置 Channel 不可写（因为 Epoll 采用的是 level trigger），避免 busy loop。
                // 还有调用写完成回调函数。
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
                // 如果连接正在关闭，则调用 shutdownInLoop() ，继续执行关闭过程。
                if (state_ == kDisconnecting) {
                    // 该函数作为 Channel 的写回调函数，在 IO 线程中执行。
                    shutdownInLoop();
                }
            } else {
                debug() << "I am going to write more data" << std::endl;
            }
        } else {
            debug() << "TcpConnection::handleWrite" << std:: endl;
        }
    } else {
        debug(LogLevel::ERROR) << "Connection is down, no more writing" << std::endl;
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    debug() << "TcpConnection::handleClose state = " << state_ << std::endl;
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    // 不关闭 socket fd，让它（Socket 对象）自己析构，从而我们可以轻松地定位到内存泄漏。
    channel_->disableAll();
    // 该回调实际是 TcpServer::removeConnection。
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError() {
    int err = socket_->getSocketError();
    debug(LogLevel::ERROR) << "TcpConnection::handleError [" << name_
                                 << "] - SO_ERROR = " << err << std::endl;
}

void TcpConnection::sendInLoop(const std::string &message) {
    loop_->assertInLoopThread();
    ssize_t n = 0;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        // 如果 Channel 当前不在写数据以及输出缓冲区没有可读数据，则尝试直接发送数据。
        n = ::write(socket_->fd(), message.data(), message.size());
        if (n >= 0) {
            if (static_cast<size_t>(n) < message.size()) {
                // 只发送了一部分数据
                debug() << "I am going to write more data" << std::endl;
            } else {
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
        } else {
            // 发送数据出错
            n = 0;
            if (errno != EWOULDBLOCK) {
                debug(LogLevel::ERROR) << "TcpConnection::sendInLoop" << std::endl;
            }
        }

        assert(n >= 0);
        // 只发送了一部分数据，剩余的数据将被放入输出缓冲区中，
        // 并开始关注写事件，以后在 handleWrite() 中发送剩余的数据。
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