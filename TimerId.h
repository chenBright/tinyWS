#ifndef TINYWS_TIMERID_H
#define TINYWS_TIMERID_H

#include <memory>

#include "Timer.h"

namespace tinyWS {
    /**
     * TODO 存在一个问题：TimerId 所指 old Timer 已被释放，新创建了的一个 new Timer 与 old Timer 地址和到期时间都一样的话，可能会有错误注销 new Timer 的操作
     * 解决方案：使用一个原子计数器创建一个序列号，用于唯一表示 TimerId
     */


    /**
     * Timer 对象是非线程安全的，不暴露给用户，
     * 只向用户提供 TdmerId 对象，用于识别定时器（主要用于注销定时器队列中定时器）
     */
    class TimerId {
    public:
        TimerId() : timer_(std::weak_ptr<Timer>()), timeout_(0) {}
        TimerId(const std::weak_ptr<Timer> &timer)
                : timer_(timer),
                  timeout_(timer.lock() ? timer.lock()->getExpiredTime() : 0) {
//        if (auto temp = timer.lock()) {
//            timeout_ = temp->getExpiredTime();
//        } else {
//            timeout_ = 0;
//        }
        }

        // 使用合成的拷贝函数、析构函数和赋值函数

        // 友元类
        friend class TimerQueue;

    private:
        std::weak_ptr<Timer> timer_; // 定时器
        Timer::TimeType timeout_; // 到期时间
    };
}


#endif //TINYWS_TIMERID_H
