#ifndef tinyWS_process1_H
#define tinyWS_process1_H

#include <array>
#include <string>
#include <functional>

#include "../base/noncopyable.h"
#include "SocketPair.h"
#include "../base/Signal.h"

namespace tinyWS_process1 {

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
        SignalManager signalManager_;

        ChildConnectionCallback childConnectionCallback_;

    public:
        explicit Process(int fds[2]);

        ~Process();

        void start();

        bool started() const;

        void setAsChild(int port);

        void setSignalHandlers();

        void setChildConnectionCallback(const ChildConnectionCallback& cb);

        pid_t getPid() const;

    private:
        void newConnection(int sockfd);

        static void childSignalHandler(int signo);
    };
}


#endif //tinyWS_process1_H
