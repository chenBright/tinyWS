#include "Timer.h"

#include <sys/time.h>

#include <memory>

using namespace tinyWS;

Timer::Timer(const Timer::TimeCallback &cb, TimeType timeout, TimeType interval)
    : timeCallback_(cb),
      expiredTime_(timeout),
      interval_(interval),
      repeat_(interval > 0) {

}

void Timer::run() const {
    timeCallback_();
}

Timer::TimeType Timer::getExpiredTime() {
    return expiredTime_;
}

void Timer::updateExpiredTime(TimeType timeout) {
    expiredTime_ = timeout;
}

bool Timer::repeat() const {
    return repeat_;
}

bool Timer::isVaild() {
    return expiredTime_ >= Timer::now();
}

Timer::TimeType Timer::invaild() {
    return 0;
}

void Timer::restart(TimeType now) {
    if (repeat_) { // 周期执行，则当前时间 + 周期
        expiredTime_ = now + interval_;
    } else { // 如果不是周期执行，则不能重设定时器到期时间
        expiredTime_ = invaild();
    }
}

Timer::TimeType Timer::now() {
    /**
     * struct timeval {
     *     time_t       tv_sec;   // seconds since Jan. 1, 1970
     *     suseconds_t  tv_usec;  // and microseconds
     * };
     */
    std::shared_ptr<timeval> tv(new timeval());
    // SUSv4 指定 gettimeofday 函数现已弃用
    gettimeofday(tv.get(), nullptr);
    return static_cast<int64_t >(tv->tv_sec * Timer::kMicroSecondsPerSecond + tv->tv_usec);
}

