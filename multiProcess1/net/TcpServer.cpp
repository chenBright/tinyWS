#include "TcpServer.h"

#include <cstdio>
#include <cassert>

#include <functional>
#include <iostream>
#include <string>

#include "EventLoop.h"
#include "Acceptor.h"
#include "ProcessPool.h"
#include "InternetAddress.h"
#include "TimerId.h"

using namespace tinyWS_process1;
using namespace std::placeholders;

TcpServer::TcpServer(const InternetAddress &address, const std::string& name)
                     : loop_(new EventLoop()),
                       name_(name),
                       acceptor_(new Acceptor(loop_, address)),
                       processPool_(new ProcessPool(loop_)),
                       nextConnectionId_(1) {

    acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnectionInParent, this, _1, _2));

    processPool_->setForkFunction(std::bind(&TcpServer::clearInSubProcess, this, _1));
}

TcpServer::~TcpServer() {
//    std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing" << std::endl;
    for (const auto& connection : connectionMap_) {
        connection.second->connectionDestroyed();
    }
}

EventLoop* TcpServer::getLoop() const {
    return loop_;
}

void TcpServer::setProcessNum(int processNum) {
    processPool_->setProcessNum(processNum);
}

void TcpServer::start() {
    if (!started_) {
        started_ = true;
        acceptor_->listen();
        // 一定要在 ProcessPool 之前 listen()。
        // 否则，将无法 listen 端口。
        // 因为程序会一直处在事件循环中，知道程序结束。
        processPool_->start();
    }
}

void TcpServer::setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback& cb) {
    messageCallback_ = cb;
}

TimerId TcpServer::runAt(TimeType runTime, const Timer::TimerCallback& cb) {
    return loop_->runAt(runTime, cb);
}

TimerId TcpServer::runAfter(TimeType delay, const Timer::TimerCallback& cb) {
    return loop_->runAfter(delay, cb);
}

TimerId TcpServer::runEvery(TimeType interval, const Timer::TimerCallback& cb) {
    return loop_->runEvery(interval, cb);
}

void TcpServer::newConnectionInParent(Socket socket, const InternetAddress& peerAddress) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", nextConnectionId_);
    ++nextConnectionId_;
    std::string connectionName = name_ + buf;

//    std::cout << "TcpServer::newConnectionInParent [" << name_
//              << "] - new connection [" << connectionName
//              << "] from " << peerAddress.toIPPort() << std::endl;

    // 发送新连接的 socket 给子进程
    processPool_->sendToChild(std::move(socket));

//    单进程代码
//    InternetAddress localAddress(InternetAddress::getLocalAddress(socket.fd()));
//    InternetAddress peerAddress1(InternetAddress::getPeerAddress(socket.fd()));
//
//    auto connection = std::make_shared<TcpConnection>(
//            loop_,
//            connectionName,
//            std::move(socket),
//            localAddress,
//            peerAddress1);
//    connectionMap_[connectionName] = connection;
//    connection->setTcpNoDelay(true);
//    connection->setConnectionCallback(connectionCallback_);
//    connection->setMessageCallback(messageCallback_);
//    connection->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
//
//    connection->connectionEstablished();
}

void TcpServer::newConnectionInChild(EventLoop* loop, Socket socket) {
    std::string connectionName = name_ +
                                 " subprocess" +
                                 std::to_string(getpid()) +
                                 "_connection_" +
                                 std::to_string(nextConnectionId_);
    ++nextConnectionId_;

    InternetAddress localAddress(InternetAddress::getLocalAddress(socket.fd()));
    InternetAddress peerAddress(InternetAddress::getPeerAddress(socket.fd()));

//    std::cout << "TcpServer::newConnectionInChild [" << name_
//              << "] - new connection [" << connectionName
//              << "] from " << peerAddress.toIPPort() << std::endl;

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
    connection->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));

    connection->connectionEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& connection) {
//        std::cout << "TcpServer::removeConnection [" << name_
//                  << "] - connection " << connection->name() << std::endl;

        size_t n = connectionMap_.erase(connection->name());

        assert(n == 1);
        (void)(n);

        connection->connectionDestroyed();
}

inline void TcpServer::clearInSubProcess(bool isParent) {
    if (!isParent) {
        // 将子进程中多余的资源释放了。

        // 如果析构了 acceptor_，会导致父进程无法接受到请求。
        // fork 之后，所有进程的 epollfd 都是指向同一文件，listened sockfd 也是。
        // 如果析构了 acceptor_，会导致在 epoll 中删除 listened sockfd，
        // 不监听 listened sockfd 的 IO 事件，从而导致父进程不能监听 listened sockfd。
        // 所以，只能关闭 listened sockfd。
//        acceptor_->~Acceptor();
        close(acceptor_->getSockfd());
        delete loop_;
        // 设置子进程接受到新连接时的回调函数
        processPool_->setChildConnectionCallback(
                std::bind(&TcpServer::newConnectionInChild, this, _1, _2));
    }
}
