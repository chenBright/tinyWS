#include "Timer.h"

#include <sys/time.h>

#include <memory>

using namespace tinyWS_thread;

AtomicInt64 Timer::s_numCreated_;

Timer::Timer(const Timer::TimerCallback &cb, TimeType timeout, TimeType interval)
    : timerCallback_(cb),
      expiredTime_(timeout),
      interval_(interval),
      repeat_(interval > 0),
      sequence_(s_numCreated_.incrementAndGet()) {

}

void Timer::run() const {
    timerCallback_();
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

int64_t Timer::getSequence() const {
    return sequence_;
}

bool Timer::isValid() {
    return expiredTime_ >= Timer::now();
}

Timer::TimeType Timer::invalid() const {
    return 0;
}

void Timer::restart(TimeType now) {
    if (repeat_) {
        // 周期执行，则当前时间 + 周期
        expiredTime_ = now + interval_;
    } else {
        // 如果不是周期执行，则不能重设定时器到期时间
        expiredTime_ = invalid();
    }
}

Timer::TimeType Timer::now() {
    // struct timeval {
    //     time_t       tv_sec;   // seconds since Jan. 1, 1970
    //     suseconds_t  tv_usec;  // and microseconds
    //     };
//    std::shared_ptr<timeval> tv(new timeval());
    std::shared_ptr<timeval> tv(std::make_shared<timeval>());

    // SUSv4 指定 gettimeofday() 函数现已弃用。
    // gettimeofday() 不是系统调用，是在用户态实现的，没有上下文切换和陷入内核的开销。
    // 精度为1纳秒。
    gettimeofday(tv.get(), nullptr);

    return static_cast<int64_t >(tv->tv_sec * Timer::kMicroSecondsPerSecond + tv->tv_usec);
}

int64_t Timer::createNum() {
    return s_numCreated_.get();
}
