#ifndef TINYWS_TIMERQUEUE_H
#define TINYWS_TIMERQUEUE_H

#include <cstdint>

#include <set>
#include <vector>
#include <memory>

#include "base/noncopyable.h"
#include "Timer.h"
#include "TimerId.h"
#include "Channel.h"


namespace tinyWS {
    class EventLoop;

    class TimerQueue : noncopyable {
    public:
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();

        TimerId addTimer(const Timer::TimeCallback &cb, Timer::TimeType timeout, Timer::TimeType interval);
        void cancel(TimerId timerId);

    private:
        typedef std::pair<Timer::TimeType , std::shared_ptr<Timer> > Entry;

        /**
         * TODO
         * set 默认升序排序（按照定时器过期时间排序）
         * 方便 insert 函数判断插入定时器是否为最早到期的和获取到期定时器
         */
        typedef std::set<Entry> TimerList;

        EventLoop *loop_; // 所属事件循环

        /**
         * TimerQueue 只关注最早到期时间，所以只使用一个 timerfd 即可。
         * 当最早的定时器到期后，获取所有到期的定时器并处理。
         * 获取所有而不是一个原因：
         * 1. 定时器队列中，可能有多个定时器的到期时间都等于最早到期时间；
         * 2. 可能存在周期处理的定时器；
         * 3. 由于调用函数或者陷入内核需要时间，在此段时间内，也有定时器到期了，尽早处理到期定时器
         */
        const int timerfd_; // 时间描述符
        Channel timerfdChannel;
        TimerList timers_; // 定时器集合，定时器根据过期时间升序排序

        // 用于防止定时器"自注销"
        bool callingExpiredTimers_; // 是否在处理到期定时器
        TimerList cancelingTimers_; // "自注销"定时器集合

        /**
         * 在 EventLoop 中往定时器队列添加定时器
         * @param timer 定时器
         */
        void addTimerInLoop(std::shared_ptr<Timer> timer);

        /**
         * 在 EventLoop 中注销定时器
         * @param timer
         */
        void cancelInLoop(std::shared_ptr<Timer> timer);

        /**
         * 定时器到期，处理到期的定时器
         */
        void handleRead();

        /**
         * 获取所有到期定时器，并从定时器队列中移除定时器
         * @param now 到期时间
         * @return 到期定时器
         */
        std::vector<Entry> getExpired(Timer::TimeType now);
        void reset(const std::vector<Entry> &expired, Timer::TimeType now);

        bool insert(std::shared_ptr<Timer> timer);
    };
}

#endif //TINYWS_TIMERQUEUE_H
