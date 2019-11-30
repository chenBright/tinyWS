#ifndef TINYWS_PROCESS_H
#define TINYWS_PROCESS_H

#include <sched.h>

#include <array>
#include <string>
#include <functional>

#include "../base/noncopyable.h"
#include "SocketPair.h"

namespace tinyWS_process {

    class EventLoop;
    class Socket;

    class Process : public noncopyable {
    public:
        using ProcessFunction = std::function<void(int)>;
        using ChildConnectionCallback = std::function<void(EventLoop*, Socket)>;

    private:
        EventLoop* loop_;
        bool running_;
        pid_t pid_;
        SocketPair pipe_;

        ChildConnectionCallback childConnectionCallback_;

    public:
        Process(int fds[2]);

        ~Process();

        void start();

        bool started() const;

        void setAsChild(int port);

// TODO 信号处理

//        int wait();

        void setChildConnectionCallback(const ChildConnectionCallback& cb);

        pid_t getPid() const;

    private:
        void newConnection(int sockfd);
    };
}


#endif //TINYWS_PROCESS_H
