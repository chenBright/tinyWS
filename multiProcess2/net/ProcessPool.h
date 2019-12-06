#ifndef tinyWS_process2POOL_H
#define tinyWS_process2POOL_H

#include <utility>
#include <functional>
#include <memory>
#include <vector>
#include <deque>
#include <string>

#include "../base/Signal.h"

namespace tinyWS_process2 {

    class EventLoop;
    class Socket;

    class ProcessPool {
    public:
        using ForkCallback = std::function<void(bool)>;

    private:
        EventLoop* baseLoop_; // 父进程事件循环
        std::vector<pid_t> pids_;
        int processNum_;

//        std::string name_;
        bool running_;

        SignalManager signalManager_;

        ForkCallback forkFunction_;

    public:
        explicit ProcessPool(EventLoop* loop);

        explicit ProcessPool(int processNum);

        ~ProcessPool();

        void start();

        void killAll();

        void setForkFunction(const ForkCallback& cb);

        void setParentSignalHandlers();

        void setChildSignalHandlers();

    private:
        void createChildProcess(int processNum);

        void parentStart();

        void clearDeadChild();

        static void parentSignalHandler(int signo);

        static void childSignalHandler(int signo);
    };
}


#endif //tinyWS_process2POOL_H
