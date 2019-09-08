#ifndef TINYWS_TIMERID_H
#define TINYWS_TIMERID_H

#include <memory>

#include "Timer.h"

namespace tinyWS {
    // Timer 对象是非线程安全的，不暴露给用户，
    // 只向用户提供 TdmerId 对象，用于识别定时器（主要用于注销定时器队列中定时器）
    class TimerId {
    public:
        TimerId() : timer_(std::weak_ptr<Timer>()), sequence_(0) {}
        explicit TimerId(const std::weak_ptr<Timer> &timer)
                : timer_(timer),
                  sequence_(timer.lock() ? timer.lock()->getSequence() : 0) {
        }

        // 使用合成的拷贝函数、析构函数和赋值函数

        // 友元类
        friend class TimerQueue;

    private:
        std::weak_ptr<Timer> timer_;    // 定时器
        int64_t sequence_;              // 定时器序列号
    };
}


#endif //TINYWS_TIMERID_H
