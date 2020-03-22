#include "Acceptor.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include <functional>
#include <utility>

#include "../base/Logger.h"
#include "EventLoop.h"
#include "InternetAddress.h"

using namespace tinyWS_thread;

Acceptor::Acceptor(EventLoop *loop, const InternetAddress &listenAddress)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop_, acceptSocket_.fd()),
      isListening_(false) {
    // 设置端口复用、绑定地址、设置"读"回调函数
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddress);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::hadleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
}

bool Acceptor::isListening() const {
    return isListening_;
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    isListening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

int Acceptor::createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        debug(LogLevel::ERROR) << "sockets::createNonblockingOrDie" << std::endl;
    }
    return sockfd;
}

void Acceptor::hadleRead() {
    loop_->assertInLoopThread();
    InternetAddress peerAddress;
    // 每次只 accept 一次
    Socket connectionSocket(acceptSocket_.accept(&peerAddress));
    if (connectionSocket.fd() >= 0) {
        if (newConnectionCallback_) {
            // 移动 Socket，保证资源的安全释放
            newConnectionCallback_(std::move(connectionSocket), peerAddress);
        }
    }

    // 一直 accept 到 EAGAIN 为止。这样需要多一次系统调用。
    // TODO 测试新能
//    while (auto sockfd = acceptSocket_.accept(&peerAddress)) {
//        if (sockfd < 0) {
//            return;
//        }
//
//        Socket connectionSocket(sockfd);
//        if (newConnectionCallback_) {
//            newConnectionCallback_(std::move(connectionSocket), peerAddress);
//        }
//    }
}

