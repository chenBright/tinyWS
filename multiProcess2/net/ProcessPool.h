#ifndef TINYWS_PROCESSPOOL_H
#define TINYWS_PROCESSPOOL_H

#include <utility>
#include <functional>
#include <memory>
#include <vector>
#include <deque>
#include <string>

#include "Process.h"
#include "../base/Signal.h"

namespace tinyWS_process {

    class EventLoop;
    class SocketPair;
    class Socket;

    class ProcessPool {
    public:
        using ForkCallback = std::function<void(bool)>;

    private:
        EventLoop* baseLoop_; // 父进程事件循环
        std::vector<std::shared_ptr<SocketPair>> pipes_;
        std::vector<pid_t> pids_;
        int processNum_;

//        std::string name_;
        bool running_;
        int next_;

        SignalManager signalManager_;

        ForkCallback forkFunction_;
        Process::ChildConnectionCallback childConnectionCallback_;

    public:
        explicit ProcessPool(EventLoop* loop);

        explicit ProcessPool(int processNum);

        ~ProcessPool();

        void setProcessNum(int processNum);

        void start();

        void killAll();

        void killSoftly();

        void sendToChild(Socket socket);

        void setForkFunction(const ForkCallback& cb);

        void setSignalHandlers();

        void setChildConnectionCallback(const Process::ChildConnectionCallback& cb);

        void newChildConnection(EventLoop* loop, Socket socket);

    private:
        void createChildAndSetParent(int processNum);

        pid_t createChildProcess(int fds[2]);

        void addChildInfoToParent(pid_t childPid, int fds[2]);

        void parentStart();

        void clearDeadChild();

        static void parentSignalHandler(int signo);
    };
}


#endif //TINYWS_PROCESSPOOL_H
