#include "Process.h"

#include <cassert>
#include <csignal>
#include <unistd.h> // getpid
#include <sys/socket.h>
#include <sys/wait.h>

#include <iostream>
#include <vector>
#include <algorithm>

#include "EventLoop.h"
#include "Socket.h"
#include "InternetAddress.h"
#include "status.h"

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
        if (status_terminate || status_quit_softly || status_restart || status_reconfigure) {
            std::cout << "subprocess(" << getpid() << ") quit" << std::endl;
            running_ = false;
        }
    }
}

bool Process::started() const {
    return running_;
}

void Process::setAsChild(int port) {
    pipe_.setChildSocket();
    pipe_.setReceiveFdCallback(std::bind(&Process::newConnection, this, _1));
}

void Process::setSignalHandlers() {
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

void Process::childSignalHandler(int signo) {
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
