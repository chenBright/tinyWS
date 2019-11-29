#include "ProcessPool.h"

#include <cassert>
#include <cstdio>
#include <unistd.h> // getpid
#include <sys/socket.h>

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "Process.h"
#include "SocketPair.h"
#include "../base/utility.h"

using namespace std::placeholders;
using namespace tinyWS_process;

ProcessPool::ProcessPool(EventLoop* loop)
      : baseLoop_(loop),
        process_(nullptr),
        running_(false) {

}

ProcessPool::~ProcessPool() {
    std::cout << "class ProcessPoll destructor\n";
}

void ProcessPool::createProccesses(int numProcesses) {
    assert(numProcesses > 0);

    pipes_.reserve(numProcesses);
    pids_.reserve(numProcesses);

    running_ = true;

    for (int i = 0; i < numProcesses; ++i) {
        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            std::cout << "[processpool] socketpair error" << std::endl;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::cout  << "[processpool] fork error" << std::endl;
        } else if (pid == 0) {
            // 子进程
            process_ = make_unique<Process>(fds);
            process_->setAsChild(static_cast<int>(getpid()));
            // TODO 信号处理
            
            start();
            return;
        } else {
            // 父进程
            std::cout << "[processpool] create process(" << pid << ")" << std::endl;

            pids_.push_back(pid);

            auto pipe = make_unique<SocketPair>(baseLoop_, fds);
            pipes_.push_back(pipe);
            pipe->setParentSocket(static_cast<int>(pids_[i]));

        }

        // 父进程
        assert(pids_.size() == pipes_.size());
    }
}

void ProcessPool::start() {
    if (process_ == nullptr) {
        // 父进程
        assert(pids_.size() == pipes_.size());

//        baseLoop_->loop();

        // TODO 信号处理
    } else {
        // 子进程
        process_->start();
    }
}

void ProcessPool::writeToChild(int nChild, int fd) {
    assert(nChild > 0);

    // TODO write file descriptor to child
//    pipes_[nChild - 1]->writeToChild()
}
