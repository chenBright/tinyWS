#include "TcpServer.h"

#include <cstdio>
#include <cassert>

#include <functional>

#include <iostream>

#include "EventLoop.h"
#include "Acceptor.h"
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
                       nextConnectionId_(1) {

    acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnection, this, _1, _2));
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
    acceptor_->listen();
}

void TcpServer::setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
}

void TcpServer::newConnection(Socket socket, const InternetAddress& peerAddress) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", nextConnectionId_);
    ++nextConnectionId_;
    std::string connectionName = name_ + buf;

    std::cout << "TcpServer::newConnection [" << name_
              << "] - new connection [" << connectionName
              << "] from " << peerAddress.toIPPort() << std::endl;

    InternetAddress localAddress(InternetAddress::getLocalAddress(socket.fd()));

    auto connection = std::make_shared<TcpConnection>(loop_,
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
        std::cout << "TcpServer::removeConnectionInLoop [" << name_
                  << "] - connection " << connection->name() << std::endl;

        size_t n = connectionMap_.erase(connection->name());

        assert(n == 1);
        (void)(n);

        connection->connectionDestroyed();
}
