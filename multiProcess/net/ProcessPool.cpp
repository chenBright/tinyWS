#include "ProcessPool.h"

#include <cassert>
#include <cstdio>
#include <unistd.h> // getpid
#include <sys/socket.h>
#include <cstdlib> // exit
#include <wait.h>

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "SocketPair.h"
#include "Socket.h"
#include "status.h"
extern int tinyWS_process::status_quit_softly; //QUIT
extern int tinyWS_process::status_terminate;   //TERM,INT
extern int tinyWS_process::status_exiting;
extern int tinyWS_process::status_reconfigure; //HUP,reboot
extern int tinyWS_process::status_child_quit;  //CHLD

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
            process.setSignalHandlers(); // 信号处理
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
            setSignalHandlers();
        }

        // 父进程
        assert(pids_.size() == pipes_.size());
    }

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
    pipes_.clear();
    pids_.clear();
}

void ProcessPool::killSoftly() {
    std::cout << "[parent] kill " << pids_.size() << " chilern softly" << std::endl;
    for (const auto& pid : pids_) {
        std::cout << "[parent] kill child(" << pid << ") softly" << std::endl;
        int result = ::kill(pid, SIGTERM);
        if (result == 0) {
            std::cout << "[parent] kill child (" << pid << ") softly successfully" << std::endl;
        }
    }
}

void ProcessPool::sendToChild(Socket socket) {
    pipes_[next_]->sendFdToChild(std::move(socket));
    next_ = (next_ + 1) % processNum_;
}

void ProcessPool::setForkFunction(const ForkCallback& cb) {
    forkFunction_ = cb;
}

void ProcessPool::setSignalHandlers() {
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

void ProcessPool::setChildConnectionCallback(const Process::ChildConnectionCallback& cb) {
    childConnectionCallback_ = cb;
}

void ProcessPool::newChildConnection(EventLoop* loop, Socket socket) {
    if (childConnectionCallback_) {
        childConnectionCallback_(loop, std::move(socket));
    }
}

void ProcessPool::parentStart() {
    int status = 1;
    while (status) {
        baseLoop_->loop();
        if (status_terminate || status_quit_softly) {
            std::cout << "[parent]:(term/stop)I will kill all chilern" << std::endl;
            killAll();
            status = 0;
            return;
        }
        if (status_restart || status_reconfigure) {
            std::cout << "[parent]:(restart/reload)quit and restart parent process's eventloop" << std::endl;
            status_restart = status_reconfigure = 0;
            // TODO
        }
        if (status_child_quit) {
            clearDeadChild();
            status_child_quit = 0;
            assert(pipes_.size() == pids_.size());
        }
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
            pipes_.erase(pipes_.begin() + (it - pids_.begin()));
            it = pids_.erase(it);
        } else {
            ++it;
        }
    }
    assert(pipes_.size() == pids_.size());
}

void ProcessPool::destroyProcess(pid_t pid) {
    auto it = std::find(pids_.begin(), pids_.end(), pid);
    pipes_.erase(pipes_.begin() + (it - pids_.begin()));
    pids_.erase(it);
    --processNum_;
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
