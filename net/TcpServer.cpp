#include "TcpServer.h"

#include <iostream>
#include <functional>

#include "../base/Logger.h"
#include "Acceptor.h"
#include "InternetAddress.h"
#include "EventLoop.h"

using namespace tinyWS;
using namespace std::placeholders;

TcpServer::TcpServer(EventLoop *loop, const InternetAddress &address, const std::string &name)
    : loop_(loop),
      name_(name),
      acceptor_(new Acceptor(loop, address)),
      threadPool_(new EventLoopThreadPool(loop)),
      started_(),
      nextConnectionId_(1) {

    acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    debug() << "TcpServer::~TcpServer [" << name_ << "] destructing" << std::endl;
    for (const auto &connection : connectionMap_) {
        connection.second->getLoop()->runInLoop(
                std::bind(&TcpConnection::connectionDestroyed, connection.second.get()));
    }
}

EventLoop* TcpServer::getLoop() const {
    return loop_;
}

void TcpServer::setThreadNumber(int threadNumber) {
    assert(threadNumber >= 0);
    threadPool_->setThreadNum(threadNumber);
}

void TcpServer::start() {
    if (started_.getAndSet(1) == 0) {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(
                std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
}

void TcpServer::setThreadInitCallback(const ThreadInitCallback &cb) {
    threadInitCallback_ = cb;
}

void TcpServer::newConnection(Socket socket, const InternetAddress &peerAddress) {
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnectionId_);
    ++nextConnectionId_;
    std::string connectionName = name_ + buf;

    debug() << "TcpServer::newConnection [" << name_
            << "] - new connection [" << connectionName
            << "] from " << peerAddress.toIpPort() << std::endl;

    InternetAddress localAddress(InternetAddress::getLocalAddress(socket.fd()));

    // ioLoop 和 loop_ 线程切换都发生在连接建立和断开的时刻，则不影响正常业务的性能。
    EventLoop *ioLoop = threadPool_->getNextLoop();
    TcpConnectionPtr connection(
            new TcpConnection(ioLoop,
                              connectionName,
                              std::move(socket),
                              localAddress,
                              peerAddress));
    connectionMap_[connectionName] = connection;
    // 设置回调函数
    connection->setConnectionCallback(connectionCallback_);
    connection->setMessageCallback(messageCallback_);
    connection->setCloseCallback(
            std::bind(&TcpServer::removeConnection, this, _1));
    // 在 IO 线程执行
    ioLoop->runInLoop(
            std::bind(&TcpConnection::connectionEstablished, connection));
}

void TcpServer::removeConnection(const TcpConnectionPtr &connection) {
    // 因为还要从 connectionMap_ 中移除 TcpConnection，
    // 所以将实际的移除工作转移到 IO 线程中。
    loop_->runInLoop(
            std::bind(&TcpServer::removeConnectionInLoop, this, connection));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &connection) {
    loop_->assertInLoopThread(); // 确保在 IO 线程中
    debug() << "TcpServer::removeConnectionInLoop [" << name_
            << "] - connection " << connection->name() << std::endl;
    size_t n = connectionMap_.erase(connection->name());

    assert(n == 1);
    (void)n;

    // ioLoop 和 loop_ 线程切换都发生在连接建立和断开的时刻，则不影响正常业务的性能。
    EventLoop *ioLoop = connection->getLoop();
    ioLoop->queueInLoop(
            std::bind(&TcpConnection::connectionDestroyed, connection));
}
