#include "TcpClient.h"

#include <functional>

#include "EventLoop.h"
#include "Connector.h"
#include "Socket.h"
#include "../base/Logger.h"

using namespace tinyWS_thread;
using namespace std::placeholders;

TcpClient::TcpClient(EventLoop *loop, const InternetAddress &serverAddress,
                     const std::string &name)
                     : loop_(loop),
                       connector_(std::make_shared<Connector>(loop_, serverAddress)),
                       name_(name),
                       connectionCallback_(defaultConnectionCallback),
                       messageCallback_(defaultMessageCallback),
                       retry_(false),
                       connect_(true),
                       nextConnectionId_(1) {

    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));

//    debug() << "TcpClient::TcpClient[" << name_
//            << "] - connector " << connector_.get();
}

TcpClient::~TcpClient() {
//    debug() << "TcpClient::~TcpClient[" << name_
//            << "] - connector " << connector_.get();

    TcpConnectionPtr connection;
    {
        MutexLockGuard lock(mutex_);
        connection = connection_;
    }

    if (connection) {
        CloseCallback cb = std::bind(&TcpClient::removeConnection, this, _1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, connection, cb));
    } else {
        connector_->stop();
//        loop_->runAfter(1, std::bind(&removeConnection, connector_));
    }
}

void TcpClient::connect() {
//    debug() << "TcpClient::connect[" << name_ << "] - connecting to "
//            << connector_->serverAddress().toIPPort();

    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

TcpConnectionPtr TcpClient::connection() const {
    MutexLockGuard lock(mutex_);
    return connection_;
}

bool TcpClient::retry() const {
    return retry_;
}

void TcpClient::enableRetry() {
    retry_ = true;
}

void TcpClient::setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
}

void TcpClient::setMessageCallback(const MessageCallback &cb) {
    messageCallback_ = cb;
}

void TcpClient::setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
}

void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InternetAddress peerAddress(InternetAddress::getLocalAddress(sockfd));
    InternetAddress localAddress(InternetAddress::getLocalAddress(sockfd));

    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnectionId_);
    ++nextConnectionId_;
    std::string connectionName = name_ + buf;


    auto connection = std::make_shared<TcpConnection>(loop_,
                                                      connectionName,
                                                      std::move(Socket(sockfd)),
                                                      localAddress,
                                                      peerAddress);

    // 设置回调函数
    connection->setConnectionCallback(connectionCallback_);
    connection->setMessageCallback(messageCallback_);
    connection->setCloseCallback(
            std::bind(&TcpClient::removeConnection, this, _1));
}

void TcpClient::removeConnection(const TcpConnectionPtr &conntion) {
    loop_->assertInLoopThread();
    assert(loop_ == conntion->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conntion);
        connection_.reset();
    }

    loop_->queueInLoop(
            std::bind(&TcpConnection::connectionDestroyed, conntion));
    if (retry_ && connect_) {
//        debug() << "TcpClient::connect[" << name_ << "] - Reconnecting to "
//                << connector_->serverAddress().toIPPort();
        connector_->restart();
    }
}
