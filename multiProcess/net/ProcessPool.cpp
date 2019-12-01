#include "ProcessPool.h"

#include <cassert>
#include <cstdio>
#include <unistd.h> // getpid
#include <sys/socket.h>
#include <cstdlib> // exit

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "SocketPair.h"
#include "Socket.h"
#include "../base/utility.h"

using namespace std::placeholders;
using namespace tinyWS_process;

ProcessPool::ProcessPool(EventLoop* loop)
      : baseLoop_(loop),
        processNum_(1),
        running_(false),
        next_(0) {

}

ProcessPool::~ProcessPool() {
    std::cout << "class ProcessPoll destructor" << std::endl;
}

void ProcessPool::setProcessNum(int processNum) {
    processNum_ = processNum;
}

void ProcessPool::start() {
    pipes_.reserve(processNum_);
    pids_.reserve(processNum_);

    running_ = true;

    for (int i = 0; i < processNum_; ++i) {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            std::cout << "[processpool] socketpair error" << std::endl;
        }

        pid_t pid = fork();

        if (pid < 0) {
            std::cout  << "[processpool] fork error" << std::endl;
        } else if (pid == 0) {
            // 子进程

            forkFunction_(false); // fork 回调函数

            Process process(fds);
            process.setAsChild(static_cast<int>(getpid()));
            process.setChildConnectionCallback(
                    std::bind(&ProcessPool::newChildConnection, this, _1, _2));
            // TODO 信号处理

            process.start();
            exit(0);
        } else {
            // 父进程

            forkFunction_(true); // fork 回调函数

            std::cout << "[processpool] create process(" << pid << ")" << std::endl;

            pids_.push_back(pid);

            std::unique_ptr<SocketPair> pipe(new SocketPair(baseLoop_, fds));
            pipe->setParentSocket();
            pipes_.push_back(std::move(pipe));
        }

        // 父进程
        assert(pids_.size() == pipes_.size());
    }
}

void ProcessPool::sendToChild(Socket socket) {
    pipes_[next_]->sendFdToChild(std::move(socket));
    next_ = (next_ + 1) % processNum_;
}

void ProcessPool::setForkFunction(const ForkCallback& cb) {
    forkFunction_ = cb;
}

void ProcessPool::setChildConnectionCallback(const Process::ChildConnectionCallback& cb) {
    childConnectionCallback_ = cb;
}

void ProcessPool::newChildConnection(EventLoop* loop, Socket socket) {
    if (childConnectionCallback_) {
        childConnectionCallback_(loop, std::move(socket));
    }
}
