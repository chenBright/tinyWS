#include "Timer.h"

#include <sys/time.h>

#include <memory>

using namespace tinyWS_process1;

AtomicInt64 Timer::s_numCreated_;

Timer::Timer(const Timer::TimerCallback &cb, TimeType timeout, TimeType interval)
    : timerCallback_(cb),
      expiredTime_(timeout),
      interval_(interval),
      repeat_(interval_ > 0),
      sequence_(s_numCreated_.incrementAndGet()) {

}

void Timer::run() const {
    timerCallback_();
}

TimeType Timer::getExpiredTime() {
    return expiredTime_;
}

void Timer::updateExpiredTime(TimeType timeout) {
    expiredTime_ = timeout;
}

bool Timer::repeat() const {
    return repeat_;
}

int64_t Timer::getSequence() const {
    return sequence_;
}

bool Timer::isValid() const {
    return expiredTime_ >= Timer::now();
}

TimeType Timer::invalid() const {
    return 0;
}

void Timer::restart(TimeType now) {
    if (repeat_) {
        expiredTime_ = now + interval_;
    } else {
        expiredTime_ = invalid();
    }
}

TimeType Timer::now() {
    // struct timeval {
    //     time_t       tv_sec;   // seconds since Jan. 1, 1970
    //     suseconds_t  tv_usec;  // and microseconds
    //     };
    std::shared_ptr<timeval> tv(std::make_shared<timeval>());

    // SUSv4 指定 gettimeofday() 函数现已弃用。
    // gettimeofday() 不是系统调用，是在用户态实现的，没有上下文切换和陷入内核的开销。
    // 精度为1纳秒。
    gettimeofday(tv.get(), nullptr);

    return static_cast<TimeType>(tv->tv_sec * Timer::kMicroSecondsPerSecond + tv->tv_usec);
}

int64_t Timer::createNum() {
    return s_numCreated_.get();
}