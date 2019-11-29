#include "SocketPair.h"

#include <sys/socket.h>
#include <unistd.h> // close、getpid
#include <sys/types.h>

#include <iostream>

#include "EventLoop.h"
#include "TcpConnection.h"
#include "../base/utility.h"

using namespace tinyWS_process;

SocketPair::SocketPair(EventLoop* loop, int fds[2])
    : loop_(loop),
      isParent_(true) {

    fds_[0] = fds[0];
    fds[1] = fds[1];

    std::cout << "class SocketPair constructor\n";
}

SocketPair::~SocketPair() {
    std::cout << "class SocketPair destructor\n";
}

void SocketPair::setParentSocket(int port) {
    if (fds_[0] == -1 || fds_[1] == -1) {
        return;
    }

    close(fds_[1]);
    isParent_ = true;
    std::cout << "switch parent:" << getpid() << std::endl;

    std::cout << "socketpair parent set connection port:" << port << std::endl;
    InternetAddress address(port);
    // TODO name
    connection_ = make_unique<TcpConnection>(loop_, "", fds_[0], address, address);
    connection_->connectionEstablished();
}

void SocketPair::setChildSocket(int port) {
    if (fds_[0] == -1 || fds_[1] == -1) {
        return;
    }

    close(fds_[0]);
    isParent_ = true;
    std::cout << "switch child:" << getpid() << std::endl;

    std::cout << "socketpair child set connection port:" << port << std::endl;
    InternetAddress address(port);
    // TODO name
    connection_ = make_unique<TcpConnection>(loop_, "", fds_[1], address, address);
    connection_->connectionEstablished();
}

void SocketPair::writeToChild(const std::string& data) {
    if (connection_ == nullptr || !isParent_) {
        return;
    }

    std::cout << "[parent] send data:" << data << std::endl;

    connection_->send(data);
}

void SocketPair::writeToParent(const std::string& data) {
    if (connection_ == nullptr || isParent_) {
        return;
    }

    std::cout << "[child] send data:" << data << std::endl;

    connection_->send(data);
}

void SocketPair::setConnectCallback(const ConnectionCallback& cb) {
    connection_->setConnectionCallback(cb);
}

void SocketPair::setMessageCallback(const MessageCallback& cb) {
    connection_->setMessageCallback(cb);
}

void SocketPair::setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    connection_->setWriteCompleteCallback(cb);
}

void SocketPair::setCloseCallback(const CloseCallback& cb) {
    connection_->setCloseCallback(cb);
}

void SocketPair::clearSocket() {
    if (fds_[0] == -1 || fds_[1] == -1) {
        return;
    }
    connection_->shutdown();
    connection_->connectionDestroyed();
}
