#ifndef TINYWS_EVENTLOOP_H
#define TINYWS_EVENTLOOP_H

#include <unistd.h>

#include <vector>
#include <memory>

#include "Timer.h"
#include "../base/noncopyable.h"

namespace tinyWS_process2 {
    class Channel;
    class Epoll;
    class TimerQueue;
    class TimerId;

    class EventLoop : noncopyable {
    public:
        using BeforeEachLoopFunction = std::function<void()>;
        using AfterEachAcceptFunction = std::function<void()>;

    private:
        using ChannelList = std::vector<Channel*>;

        bool running_;
        pid_t pid_;
        std::unique_ptr<Epoll> epoll_;
        std::unique_ptr<TimerQueue> timerQueue_;
        ChannelList activeChannels_;

        int listenSockfd_;

        BeforeEachLoopFunction beforeEachLoopFunction_;
        AfterEachAcceptFunction afterEachAcceptFunction_;

    public:
        EventLoop();

        ~EventLoop();

        void loop();

        void quit();

        TimerId runAt(TimeType runTime, const Timer::TimerCallback& cb);

        TimerId runAfter(TimeType delay, const Timer::TimerCallback& cb);

        TimerId runEvery(TimeType interval, const Timer::TimerCallback& cb);

        void cancel(const TimerId& timerId);

        void updateChannel(Channel* channel);

        void removeChannel(Channel* channel);

        void setListenSockfd(int sockfd);

        void setBeforeEachLoopFunction(const BeforeEachLoopFunction& cb);

        void setAfterEachLoopFunction(const AfterEachAcceptFunction& cb);

    private:
        void printActiveChannels() const; // for DEBUG
    };
}


#endif //TINYWS_EVENTLOOP_H
