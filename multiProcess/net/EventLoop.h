//
// Created by 陈光明 on 2019/11/17.
//

#ifndef TINYWS_EVENTLOOP_H
#define TINYWS_EVENTLOOP_H

#include <unistd.h>

#include <vector>
#include <memory>

#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

namespace tinyWS_process {
    class Channel;
    class Epoll;
    class TimerQueue;

    class EventLoop {
    private:
        using ChannelList = std::vector<Channel*>;

        bool running_;
        pid_t pid_;
        std::unique_ptr<Epoll> epoll_;
        std::unique_ptr<TimerQueue> timerQueue_;
        ChannelList activeChannels_;

    public:
        EventLoop();

        ~EventLoop() = default;

        void loop();

        void quit();

        TimerId runAt(TimeType time, const Timer::TimerCallback& cb);

        TimerId runAfter(TimeType delay, const Timer::TimerCallback &cb);

        TimerId runEvery(TimeType interval, const Timer::TimerCallback &cb);

        void cancel(const TimerId& timerId);

        void updateChannel(Channel* channel);

        void removeChannel(Channel* channel);

    private:
        void printActiveChannels() const; // for DEBUG
    };
}


#endif //TINYWS_EVENTLOOP_H
