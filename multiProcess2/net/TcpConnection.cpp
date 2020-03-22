#include "TcpConnection.h"

#include <unistd.h> // read
#include <cassert>

#include <iostream>

#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "../base/utility.h"

using namespace tinyWS_process2;
using namespace std::placeholders;

TcpConnection::TcpConnection(EventLoop* loop,
                             std::string& name,
                             Socket socket,
                             const InternetAddress& localAddress,
                             const InternetAddress& peerAddress)
                             : loop_(loop),
                               name_(name),
                               state_(kConnecting),
                               socket_(new Socket(std::move(socket))),
                               channel_(new Channel(loop, socket_->fd())),
                               localAddress_(localAddress),
                               peerAddress_(peerAddress) {
    // 设置 Channel 的回调函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallack(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

//    std::cout << "TcpConnection::ctor[" <<  name_ << "] at " << this
//              << " fd=" << socket_->fd() << std::endl;

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    assert(state_ == kDisconnected);
}

void TcpConnection::send(const std::string& message) {
    if (state_ == kConnected) {
        ssize_t n = 0;
        if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
            n = ::write(socket_->fd(), message.data(), message.size());
            if (n >= 0) {
                if (static_cast<size_t>(n) < message.size()) {
                    // 只发送了一部分数据
//                    std::cout << "I am going to write more data" << std::endl;
                } else {
                    if (writeCompleteCallback_) {
                        writeCompleteCallback_(shared_from_this());
                    }
                }
            } else {
                // 发送数据出错
                n = 0;
                if (errno != EWOULDBLOCK) {
//                    std::cout << "TcpConnection::send" << std::endl;
                }
            }

            assert(n >= 0);
            // 只发送了一部分数据，剩余的数据将被放入输出缓冲区中，
            // 并开始关注写事件，以后在 handleWrite() 中发送剩余的数据。
            if (static_cast<size_t>(n) < message.size()) {
                outputBuffer_.append(message.data() + n, message.size() - n);
                if (!channel_->isWriting()) {
                    channel_->enableReading();
                }
            }
        }
    }
}

void TcpConnection::send(const void* message, size_t len) {
    const char* cStr = static_cast<const char*>(message);
    send(std::string(cStr, len));
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        shutdownInSocket();
    }
}

void TcpConnection::setContext(const tinyWS_process2::any &context) {
    context_ = context;
}

const tinyWS_process2::any& TcpConnection::getContext() const {
    return context_;
}

tinyWS_process2::any* TcpConnection::getMutableContext() {
    return &context_;
}


void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::setKeepAlive(bool on) {
    socket_->setKeepAlive(on);
}

void TcpConnection::setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
}

void TcpConnection::setMessageCallback(const MessageCallback& cb) {
    messageCallback_ = cb;
}

void TcpConnection::setCloseCallback(const CloseCallback& cb) {
    closeCallback_ = cb;
}

void TcpConnection::setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
}

void TcpConnection::connectionEstablished() {
    assert(state_ == kConnecting);
    setState(kConnected);

    channel_->tie(shared_from_this());
    channel_->enableReading();

    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectionDestroyed() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();

        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
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

void TcpConnection::setState(TcpConnection::StateE s) {
    state_ = s;
}

void TcpConnection::handleRead(TimeType receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(socket_->fd());
    if (n > 0) {
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
    } else if (n == 0) {
        handleClose();
    } else {
//        std::cout << "TcpConnection::handleError" << std::endl;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        ssize_t n = ::write(socket_->fd(),
                            outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if (n > 0) {
            if (outputBuffer_.readableBytes() == 0) {
                // 如果 outputBuffer_ 中没有可读数据，即数据已经发送完毕，
                // 则立即设置 Channel 不可写（因为 Epoll 采用的是 level trigger），避免 busy loop。
                // 调用写完成回调函数。
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    writeCompleteCallback_(shared_from_this());
                }
                if (state_ == kDisconnecting) {
                    shutdownInSocket();
                }
            }
        }
    }
}

void TcpConnection::handleClose() {
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();

    if (closeCallback_) {
        // 该回调实际是 TcpServer::removeConnection。
        closeCallback_(shared_from_this());
    }
}

void TcpConnection::handleError() {
    int err = socket_->getSocketError();

//    std::cout << "TcpConnection::handleError [" << name_
//              << "] - SO_ERROR = " << err << std::endl;
}

void TcpConnection::shutdownInSocket() {
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

std::string TcpConnection::stateToString() const {
    switch (state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}
