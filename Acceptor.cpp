#include "Acceptor.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <functional>
#include <utility>

#include "EventLoop.h"
#include "InternetAddress.h"

using namespace tinyWS;

Acceptor::Acceptor(EventLoop *loop, const InternetAddress &listenAdress)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop_, acceptSocket_.fd()),
      isListening_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAdress);
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
    // TODO socket 直接得到无阻塞 IO 文件描述符
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        std::cout << "sockets::createNonblockingOrDie" << std::endl;
    }
    return sockfd;
}

void Acceptor::hadleRead() {
    loop_->assertInLoopThread();
    InternetAddress peerAddress(0);
    // FIXME loop until no more
    Socket connectionSocket(acceptSocket_.accept(&peerAddress));
    if (connectionSocket.fd() >= 0) {
        if (newConnectionCallback_) {
            // 移动 Socket，保证资源的安全释放
            newConnectionCallback_(std::move(connectionSocket), peerAddress);
        }
    }
}

