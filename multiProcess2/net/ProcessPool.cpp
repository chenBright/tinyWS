#include "ProcessPool.h"

#include <cassert>
#include <unistd.h> // getpid
#include <sys/socket.h>
#include <cstdlib> // exit
#include <wait.h>

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "status.h"

using namespace std::placeholders;
using namespace tinyWS_process2;

ProcessPool::ProcessPool(EventLoop* loop)
      : baseLoop_(loop),
        processNum_(1),
        running_(false) {

}

ProcessPool::ProcessPool(int processNum)
    : processNum_(processNum) {

    createChildProcess(processNum_);

}

ProcessPool::~ProcessPool() {
    std::cout << "class ProcessPoll destructor" << std::endl;
}

void ProcessPool::start() {
    running_ = true;

    parentStart();
}

void ProcessPool::killAll() {
    std::cout << "[parent] kill " << pids_.size() << " chilern" << std::endl;
    for (const auto& pid : pids_) {
        std::cout << "[parent] kill child(" << pid << ")" << std::endl;
        int result = ::kill(pid, SIGINT);
        if (result == 0) {
            std::cout << "[parent] kill child (" << pid << ") successfully" << std::endl;
        }
    }
    pids_.clear();
}

void ProcessPool::setForkFunction(const ForkCallback& cb) {
    forkFunction_ = cb;
}

void ProcessPool::setParentSignalHandlers() {
    std::vector<Signal> signals = {
            Signal(SIGINT, "SIGINT", "kill all", &parentSignalHandler),
            Signal(SIGTERM, "SIGTERM", "kill softly", &parentSignalHandler),
            Signal(SIGCHLD, "SIGCHLD", "child dead", &parentSignalHandler),
            Signal(SIGUSR1, "SIGUSR1", "restart", &parentSignalHandler),
            Signal(SIGUSR2, "SIGUSR2", "reload", &parentSignalHandler),
            Signal(SIGQUIT, "SIGQUIT", "quit softly", &parentSignalHandler),
            Signal(SIGPIPE, "SIGPIPE", "socket close", &parentSignalHandler),
            Signal(SIGHUP, "SIGHUP", "reconfigure", &parentSignalHandler),
    };

    for (const auto& s : signals) {
        signalManager_.addSignal(s);
    }
}

void ProcessPool::setChildSignalHandlers() {
    std::vector<Signal> signals = {
            Signal(SIGINT, "SIGINT", "kill all", &childSignalHandler),
            Signal(SIGTERM, "SIGTERM", "kill softly", &childSignalHandler),
            Signal(SIGUSR1, "SIGUSR1", "restart", &childSignalHandler),
            Signal(SIGUSR2, "SIGUSR2", "reload", &childSignalHandler),
            Signal(SIGQUIT, "SIGQUIT", "quit softly", &childSignalHandler),
            Signal(SIGPIPE, "SIGPIPE", "socket close", &childSignalHandler),
            Signal(SIGHUP, "SIGHUP", "reconfugure", &childSignalHandler),
    };

    for (const auto& s : signals) {
        signalManager_.addSignal(s);
    }
}


void ProcessPool::createChildProcess(int processNum) {
    pids_.reserve(processNum);

    for (int i = 0; i < processNum; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cout  << "[processpool] fork error" << std::endl;
        } else if (pid == 0) {
            // 子进程
            std::cout << "[processpool] child process(" << getpid() << ")" << std::endl;

            setChildSignalHandlers();

            return;
        } else {
            // 父进程

//        forkFunction_(true); // fork 回调函数

            std::cout << "[processpool] " << getpid() << " create process(" << pid << ")" << std::endl;

            pids_.push_back(pid);

            setParentSignalHandlers();
        }
    }
}

pid_t ProcessPool::createNewChildProcess() {
    std::cout << "pids size: " << pids_.size() << std::endl;
    for (int i = pids_.size(); i <= processNum_; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cout  << "[processpool] fork error" << std::endl;
        } else if (pid == 0) {
            // 子进程
            std::cout << "[processpool] new child process(" << getpid() << ")" << std::endl;

            setChildSignalHandlers();

            break;
        } else {
            // 父进程

//        forkFunction_(true); // fork 回调函数

            std::cout << "[processpool] " << getpid() << " create new process(" << pid << ")" << std::endl;

            pids_.push_back(pid);
        }

        return pid;
    }
}

void ProcessPool::parentStart() {
    while (running_) {
        baseLoop_->loop();
        if (status_terminate || status_quit_softly || status_child_quit) {
            std::cout << "[parent]:(term/stop)I will kill all chilern" << std::endl;
            killAll();
            running_ = false;
            return;
        }

        if (status_restart || status_reconfigure) {
            std::cout << "[parent]:(restart/reload)quit and restart parent process's eventloop" << std::endl;
            status_restart = status_reconfigure = 0;

            // 只是单纯地重启父进程的 EVentLoop
        }

//        TODO 创建新进程，并重启 EventLoop
//        if (status_child_quit) {
//            clearDeadChild();
//            status_child_quit = 0;
//            assert(pipes_.size() == pids_.size());
//
//            // 创建新的子进程，使得子进程数等于 processNum_
//            int numToCreating = processNum_ - static_cast<int>(pids_.size());
//            createChildAndSetParent(numToCreating);
//        }
    }
}

void ProcessPool::clearDeadChild() {
    for (auto it = pids_.begin(); it != pids_.end();) {
        // kill 不会发送空信号0，但仍然执行正常的错误检查，但不发送信号。
        // 可用于检查某一特定进程是否仍然存在。
        // 参考
        // man 2 kill
        // 《APUE》P268
        // https://typecodes.com/cseries/kill0checkprocessifexist.html
        int isAlive = ::kill(*it, 0);
        if (isAlive == -1) {
            std::cout << "[parent]:clear subprocess " << *it << std::endl;
            it = pids_.erase(it);
        } else {
            ++it;
        }
    }

}

void ProcessPool::parentSignalHandler(int signo) {
    std::cout << "[parent] signal manager get signal(" << signo << ")" << std::endl;

    pid_t pid;
    int status;

    switch (signo) {
        case SIGINT:
        case SIGTERM:
            status_terminate = 1;
            std::cout << "[parent] will terminate all subprocess" << std::endl;
            break;

        case SIGQUIT:
            status_quit_softly = 1;
            std::cout << "[parent] quit softly" << std::endl;
            break;

        case SIGPIPE:
            break;

        case SIGCHLD:
            status_child_quit = 1;
            // WNOHANG：若参数 pid 指定子进程并不是立即可用，则 waitpid 不阻塞，返回 0。
            pid = ::waitpid(-1, &status, WNOHANG);
            std::cout << "[parent] collect information from child(" << pid << ")" << std::endl;
            break;

        case SIGUSR1:
            status_restart = 1;
            std::cout << "[parent] restart" << std::endl;
            break;

        case SIGUSR2:
            status_reconfigure = 1;
            std::cout << "[parent] (" << pid << ") reload" << std::endl;
            break;

        default:
            break;
    }
}

void ProcessPool::childSignalHandler(int signo) {
    pid_t pid = getpid();
    std::cout << "[child] (" << pid << ") signal manager get signal(" << signo << ")" << std::endl;

    switch (signo) {
        case SIGINT:
        case SIGTERM:
            status_terminate = 1;
            std::cout << "[child] (" << pid << ") will terminate" << std::endl;
            break;

        case SIGQUIT:
            status_quit_softly = 1;
            std::cout << "[child] (" << pid << ") will quit softly" << std::endl;
            break;

        case SIGPIPE:
            break;

        case SIGUSR1:
            status_restart = 1;
            std::cout << "[child] (" << pid << ") restart" << std::endl;
            break;

        case SIGUSR2:
            status_reconfigure = 1;
            std::cout << "[child] (" << pid << ") reload" << std::endl;
            break;

        default:
            break;
    }
}