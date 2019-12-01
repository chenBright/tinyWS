#ifndef TINYWS_TIMERQUEUE_H
#define TINYWS_TIMERQUEUE_H

#include <utility>
#include <memory>
#include <set>
#include <vector>

#include "Channel.h"
#include "../base/noncopyable.h"
#include "Timer.h"

namespace tinyWS_process {

    class EventLoop;
    class TimerId;

    class TimerQueue : noncopyable {
    private:
        using Entry = std::pair<TimeType, std::shared_ptr<Timer>>;

        using TimerList = std::set<Entry>;

        using ActiveTimer = std::pair<std::shared_ptr<Timer>, int64_t>;
        using ActiveTimerSet = std::set<ActiveTimer>;

        EventLoop *loop_;

        const int timerfd_;
        Channel timerfdChannel_;
        TimerList timers_;

        ActiveTimerSet activeTimers_;
        bool callingExpiredTimers_;
        ActiveTimerSet cancelTimers_;

    public:
        explicit TimerQueue(EventLoop *loop);

        ~TimerQueue();

        TimerId addTimer(const Timer::TimerCallback& cb, TimeType timeout, TimeType interval);

        void cancel(const TimerId& timerId);

    private:
        void handleRead();

        std::vector<Entry> getExpired(TimeType now);

        void reset(const std::vector<Entry>& expired, TimeType now);

        bool insert(const std::shared_ptr<Timer>& timer);
    };
}


#endif //TINYWS_TIMERQUEUE_H
