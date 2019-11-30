#include "Process.h"

#include <cassert>
#include <unistd.h> // getpid
#include <sys/socket.h>
#include <sys/wait.h>

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "Socket.h"
#include "InternetAddress.h"

using namespace tinyWS_process;
using namespace std::placeholders;

Process::Process(int fds[2])
    : loop_(new EventLoop()),
      running_(false),
      pid_(getpid()),
      pipe_(loop_, fds) {
    // 初始化 pipefd_
}

Process::~Process() {
    delete loop_;

    std::cout << "class Process destructor" << std::endl;
}

void Process::start() {
    assert(!running_);

    running_ = true;

    while (running_) {
        loop_->loop();
    }
}

bool Process::started() const {
    return running_;
}

void Process::setAsChild(int port) {
    pipe_.setChildSocket();
    pipe_.setReceiveFdCallback(std::bind(&Process::newConnection, this, _1));
}

//
//int Process::wait() {
//    return waitpid(pid_, nullptr, 0);
//}

void Process::setChildConnectionCallback(const ChildConnectionCallback& cb) {
    childConnectionCallback_ = cb;
}

pid_t Process::getPid() const {
    return pid_;
}

void Process::newConnection(int sockfd) {
    if (childConnectionCallback_) {
        Socket socket(sockfd);
        childConnectionCallback_(loop_, std::move(socket));
    }
}

