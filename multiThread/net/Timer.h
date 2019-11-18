#ifndef TINYWS_TIMER_H
#define TINYWS_TIMER_H

#include <cstdint>

#include <functional>

#include "../base/noncopyable.h"
#include "../base/Atomic.h"

namespace tinyWS_thread {
    // 非线程安全，不暴露给用户，只向用户提供 TdmerId 对象，用于识别定时器。
    class Timer : noncopyable {
    public:
        using TimeCallback = std::function<void()>;                 // 定时器回调函数类型
        using TimeType = int64_t;                                   // 时间数据类型

        const static TimeType kMicroSecondsPerSecond = 1000 * 1000; // 一秒有 1000 * 1000 微秒

        /**
         * 构造函数
         * @param cb 回调函数
         * @param timeout 到期时间
         * @param interval
         */
        explicit Timer(const TimeCallback &cb, TimeType timeout, TimeType interval = 0);

        /**
         * 执行回调函数
         */
        void run() const;

        /**
         * 获取到期时间
         * @return 到期时间
         */
        TimeType getExpiredTime();

        /**
         * 更新到期时间
         * @param timeout 新的到期时间
         */
        void updateExpiredTime(TimeType timeout);

        /**
         * 是否周期执行
         * @return true / false
         */
        bool repeat() const;

        /**
         * 获取定时器序列号
         * @return 序列号
         */
        int64_t getSequence() const;

        /**
         * 是否有效
         * @return true / false
         */
        bool isVaild();

        /**
         * 返回无效时间
         * @return 0
         */
        Timer::TimeType invaild() const;

        /**
         * 重设定时器到期时间
         * 只有周期执行的定时器才能重设到期时间。
         * 不是周期执行的定时器不能重设到期时间，
         * 如果调用此函数的定时器不是周期执行的定时器，
         * 其到期时间讲会设置为无效值（0）。
         * @param now 新的到期时间
         */
        void restart(TimeType now);

        /**
         * 获取当前时间距 1970年1月1日 00 : 00 : 00 的微秒数
         * @return 微秒
         */
        static TimeType now();

        static int64_t createNum();

    private:
        const TimeCallback timeCallback_;   // 定时器回调函数
        TimeType expiredTime_;              // 到期时间
        const TimeType interval_;           // 执行周期
        const bool repeat_;                 // 是否周期执行
        const int64_t sequence_;            // 定时器序列号

        static AtomicInt64 s_numCreated_;   // 序列号生成器
    };
}

#endif //TINYWS_TIMER_H
