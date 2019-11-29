#ifndef TINYWS_PROCESSPOOL_H
#define TINYWS_PROCESSPOOL_H

#include <utility>
#include <functional>
#include <memory>
#include <vector>
#include <deque>
#include <string>

namespace tinyWS_process {

    class EventLoop;
    class Process;
    class SocketPair;

    class ProcessPool {
    public:

    private:
        EventLoop* baseLoop_; // 父进程事件循环
        std::unique_ptr<Process> process_; // 子进程
        std::vector<std::unique_ptr<SocketPair>> pipes_;
        std::vector<pid_t> pids_;

//        std::string name_;
        bool running_;

    public:
        ProcessPool(EventLoop *loop);

        ~ProcessPool();

        void createProccesses(int numProcesses);

        void start();

        void writeToChild(int nChild, int fd);
    };
}


#endif //TINYWS_PROCESSPOOL_H
