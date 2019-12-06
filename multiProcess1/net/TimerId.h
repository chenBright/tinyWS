#ifndef TINYWS_TIMERID_H
#define TINYWS_TIMERID_H

#include <memory>

#include "Timer.h"

namespace tinyWS_process1 {
    class TimerId{
    public:
        friend class TimerQueue;

    private:
        std::weak_ptr<Timer> timer_;
        int64_t sequence_;

    public:
        TimerId() : sequence_(0) {}

        explicit TimerId(const std::weak_ptr<Timer>& timer)
            : timer_(timer),
              sequence_(timer.lock() ? timer.lock()->getSequence() : 0) {

        }
    };
}

#endif //TINYWS_TIMERID_H
