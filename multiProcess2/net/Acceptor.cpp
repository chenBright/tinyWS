#include "Acceptor.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include <functional>
//#include <algorithm>
#include <utility>
#include <iostream>

#include "EventLoop.h"
#include "InternetAddress.h"

using namespace tinyWS_process2;

Acceptor::Acceptor(EventLoop* loop, const InternetAddress& listenAddress)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(loop_, acceptSocket_.fd()),
      isListening_(false) {

    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddress);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::Acceptor(EventLoop* loop, Socket socket,
                   const InternetAddress &listenAddress)
        : loop_(loop),
          acceptSocket_(std::move(socket)),
          acceptChannel_(loop_, acceptSocket_.fd()),
          isListening_(false) {

    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddress);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::resetLoop(EventLoop* loop) {
    loop_ = loop;
    acceptChannel_.resetLoop(loop);
    listen();
}

void Acceptor::setNewConnectionCallback(const Acceptor::NewConnectionCallback& cb) {
    newConnectionCallback_ = cb;
}

void Acceptor::listen() {
    isListening_ = true;
    acceptSocket_.listen();
}

void Acceptor::listenInEpoll() {
    acceptChannel_.enableReading();
}

void Acceptor::unlistenInEpoll() {
    acceptChannel_.disableReading();
}

bool Acceptor::isLIstening() const {
    return isListening_;
}

int Acceptor::getSockfd() const {
    return acceptSocket_.fd();
}

int Acceptor::createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        std::cout << "sockets::createNonblockingOrDie" << std::endl;;
    }
}

void Acceptor::handleRead() {
    InternetAddress peerAddress;

    auto sockfd = acceptSocket_.accept(&peerAddress);
    std::cout << "sockfd: " << sockfd << "(" << getpid() << ")" << std::endl;
    if (sockfd < 0) {
        return;
    }

    Socket connectionSocket(sockfd);
    if (newConnectionCallback_) {
        newConnectionCallback_(std::move(connectionSocket), peerAddress);
    }
}
