#include "Process.h"

#include <cassert>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <iostream>

#include "EventLoop.h"

using namespace tinyWS_process;

Process::Process(int fds[2])
    : loop_(new EventLoop()),
      running_(false),
      pid_(getPid()),
      pipe_(loop_, fds) {
    // 初始化 pipefd_
}

Process::~Process() {
    pipe_.clearSocket();
    // TODO delete EventLoop ?
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
    pipe_.setChildSocket(port);
    // TODO 其他回调函数
}

//
//int Process::wait() {
//    return waitpid(pid_, nullptr, 0);
//}

pid_t Process::getPid() const {
    return pid_;
}

