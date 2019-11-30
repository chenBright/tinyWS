#include "TcpServer.h"

#include <cstdio>
#include <cassert>

#include <functional>

#include <iostream>

#include "EventLoop.h"
#include "Acceptor.h"
#include "ProcessPool.h"
#include "InternetAddress.h"
#include "../base/utility.h"

using namespace tinyWS_process;
using namespace std::placeholders;

TcpServer::TcpServer(EventLoop* loop,
                     const InternetAddress& address,
                     const std::string &name)
                     : loop_(loop),
                       name_(name),
                       acceptor_(make_unique<Acceptor>(loop, address)),
                       processPool_(make_unique<ProcessPool>(loop_)),
                       nextConnectionId_(1) {

    acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnectionInParent, this, _1, _2));

    processPool_->setForkFunction(std::bind(&TcpServer::clearInSubProcess, this, _1));
}

TcpServer::~TcpServer() {
    std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing" << std::endl;
    for (const auto& connection : connectionMap_) {
        connection.second->connectionDestroyed();
    }
}

EventLoop* TcpServer::getLoop() const {
    return loop_;
}

void TcpServer::start() {
    if (!started_) {

        processPool_->start();

        acceptor_->listen();
        started_ = true;
    }
}

void TcpServer::setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
}

void TcpServer::newConnectionInParent(Socket socket, const InternetAddress& peerAddress) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", nextConnectionId_);
    ++nextConnectionId_;
    std::string connectionName = name_ + buf;

    std::cout << "TcpServer::newConnectionInParent [" << name_
              << "] - new connection [" << connectionName
              << "] from " << peerAddress.toIPPort() << std::endl;

    // 发送新连接的 socket 给子进程
    processPool_->sendToChild(std::move(socket));
}

void TcpServer::newConnectionInChild(EventLoop* loop, Socket socket) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", nextConnectionId_);
    ++nextConnectionId_;
    std::string connectionName = name_ + " subprocess " + buf;

    InternetAddress localAddress(InternetAddress::getLocalAddress(socket.fd()));
    InternetAddress peerAddress(InternetAddress::getPeerAddress(socket.fd()));

    std::cout << "TcpServer::newConnectionInParent [" << name_
              << "] - new connection [" << connectionName
              << "] from " << peerAddress.toIPPort() << std::endl;

    auto connection = std::make_shared<TcpConnection>(
            loop,
            connectionName,
            std::move(socket),
            localAddress,
            peerAddress);
    connectionMap_[connectionName] = connection;
    connection->setTcpNoDelay(true);
    connection->setConnectionCallback(connectionCallback_);
    connection->setMessageCallback(messageCallback_);
    connection->setConnectionCallback(std::bind(&TcpServer::removeConnection, this, _1));

    connection->connectionEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& connection) {
        std::cout << "TcpServer::removeConnection [" << name_
                  << "] - connection " << connection->name() << std::endl;

        size_t n = connectionMap_.erase(connection->name());

        assert(n == 1);
        (void)(n);

        connection->connectionDestroyed();
}

inline void TcpServer::clearInSubProcess(bool isParent) {
    if (!isParent) {
        delete(loop_);

        auto ptr = acceptor_.release();
        delete(ptr);

        // 设置子进程接受到新连接时的回调函数
        processPool_->setChildConnectionCallback(
                std::bind(&TcpServer::newConnectionInChild, this, _1, _2));
    }
}
