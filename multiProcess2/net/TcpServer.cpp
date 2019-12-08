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
#include "status.h"

using namespace tinyWS_process2;
using namespace std::placeholders;

TcpServer::TcpServer(const InternetAddress& address, const std::string& name)
                     : socketBeforeFork_(Acceptor::createNonblocking()), // listen socket
                       processMutexLock_(),
                       isLock_(false),
                       processPool_(new ProcessPool(4)),
                       loop_(new EventLoop()),
                       acceptor_(new Acceptor(loop_, std::move(socketBeforeFork_), address)),
                       name_(name),
                       nextConnectionId_(1) {

    acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnectionInParent, this, _1, _2));

    loop_->setListenSockfd(acceptor_->getSockfd());
    loop_->setBeforeEachLoopFunction(std::bind(&TcpServer::lockAcceptor, this));
    loop_->setAfterEachLoopFunction(std::bind(&TcpServer::unlockAcceptor, this));
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
        started_ = true;
        acceptor_->listen();
//        acceptor_->listenInEpoll();

        bool running = true;
        while (running) {
            loop_->loop();

            if (status_terminate || status_quit_softly) {
                std::cout << "[parent]:(term/stop) I will kill all chilern" << std::endl;
                processPool_->killAll();
                running = false;
            }

            if (status_restart || status_reconfigure) {
                std::cout << "[parent]:(restart/reload)quit and restart parent process's eventloop" << std::endl;
                status_restart = status_reconfigure = 0;

                // 只是单纯地重启父进程的 EVentLoop
            }

            if (status_child_quit) {
                std::cout << "[parent]:child exit, I will create a new one" << std::endl;

                // 重新生成新的子进程
                pid_t pid = processPool_->createNewChildProcess();

                status_child_quit = 0;
                if (pid == 0) {
                    delete loop_;
                    loop_ = new EventLoop();
                    acceptor_->resetLoop(loop_);
                }
            }
        }
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

    std::cout << "TcpServer::newConnection ( " << getpid() << " ) [" << name_
              << "] - new connection [" << connectionName
              << "] from " << peerAddress.toIPPort() << std::endl;

    // 发送新连接的 socket 给子进程
//    processPool_->sendToChild(std::move(socket));

//    单进程代码
    InternetAddress localAddress(InternetAddress::getLocalAddress(socket.fd()));

    auto connection = std::make_shared<TcpConnection>(
            loop_,
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
        std::cout << "TcpServer::removeConnection [" << name_
                  << "] - connection " << connection->name() << std::endl;

        size_t n = connectionMap_.erase(connection->name());

        assert(n == 1);
        (void)(n);

        connection->connectionDestroyed();
}

void TcpServer::reset() {
    delete loop_;
    loop_ = new EventLoop();
}

void TcpServer::lockAcceptor() {
    bool locked = processMutexLock_.trylock();
    if (locked) {
        std::cout << "get lock in process " << getpid() << std::endl;
        isLock_ = true;
        acceptor_->listenInEpoll();
    }
}

void TcpServer::unlockAcceptor() {
    std::cout << "unlockAcceptor in process " << getpid() << std::endl;
    if (isLock_) {
        std::cout << "release lock in process " << getpid() << std::endl;

        isLock_ = false;
        acceptor_->unlistenInEpoll();
        processMutexLock_.unlock();
    }
}
