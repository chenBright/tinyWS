#ifndef TINYWS_TIMER_H
#define TINYWS_TIMER_H

#include <functional>

#include "../base/noncopyable.h"
#include "type.h"
#include "../base/Atomic.h"

namespace tinyWS_process1 {

    class Timer : noncopyable {
    public:
        using TimerCallback = std::function<void()>;

        const static TimeType kMicroSecondsPerSecond = 1000 * 1000; // 一秒有 1000 * 1000 微秒

    private:
        const TimerCallback timerCallback_;
        TimeType expiredTime_;
        const TimeType interval_;
        const bool repeat_;
        const int64_t sequence_;

        static AtomicInt64 s_numCreated_;

    public:
        Timer(const TimerCallback& cb, TimeType timeout, TimeType interval = 0);

        void run() const;

        TimeType getExpiredTime();

        void updateExpiredTime(TimeType timeout);

        bool repeat() const;

        int64_t getSequence() const;

        bool isValid() const;

        TimeType invalid() const;

        void restart(TimeType now);

        static TimeType now();

        static int64_t createNum();
    };
}


#endif //TINYWS_TIMER_H
