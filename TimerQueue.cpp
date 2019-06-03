#include "TimerQueue.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <cassert>
#include <strings.h> // bzero()

#include <iostream>
#include <functional>
#include <algorithm>
#include <utility>

#include "EventLoop.h"

using namespace tinyWS;

// timerfd 操作函数集合
namespace Timerfd {
    // TODO 学习 timerfd
    int createTimerfd() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0) {
            std::cout << "Failed in timerfd_create" << std::endl;
        }

        return timerfd;
    }
    void readTimerfd(int timerfd, Timer::TimeType now) {
        uint64_t howmany;
        ssize_t n = read(timerfd, &howmany, sizeof(howmany));
        std::cout << "TimerQueue::handleRead() " << howmany << " at " << now << std::endl;
        if (n != sizeof(howmany)) {
            std::cout << "TimerQueue::handleRead() reads " << n << " bytes instead of 8" << std::endl;
        }
    }

    timespec howMuchTimeFromNow(Timer::TimeType when) {
        Timer::TimeType microseconds = when - Timer::now();
        if (microseconds < 100) {
            microseconds = 100;
        }
        timespec ts{};
        ts.tv_sec = static_cast<time_t>(microseconds / Timer::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>((microseconds % Timer::kMicroSecondsPerSecond) * 1000);

        return ts;
   }

    void resetTimerfd(int timerfd, Timer::TimeType when) {
        // wake up loop by timerfd_settime()
        itimerspec oldValue{};
        itimerspec newValue{};
        bzero(&oldValue, sizeof(oldValue));
        bzero(&newValue, sizeof(newValue));
        newValue.it_value = howMuchTimeFromNow(when);
        // 通过 timerfd_settime 函数唤醒 IO 线程
        int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if (ret) {
            std::cout << "timerfd_settime()" << std::endl;
        }
    }
};

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(Timerfd::createTimerfd()),
      timerfdChannel(loop, timerfd_),
      timers_() {

    timerfdChannel.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel.enableReading();
}

TimerQueue::~TimerQueue() {
    close(timerfd_);
    // 直到 EventLoop 析构之前，都不能移除 channel
}

TimerId TimerQueue::addTimer(const Timer::TimeCallback &cb, Timer::TimeType timeout, Timer::TimeType interval) {
    std::shared_ptr<Timer> timer(new Timer(cb, timeout, interval));
    // 将添加定时器的实际工作转移到 IO 线程，使得不加锁也能保证线程安全性
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));

    return std::weak_ptr<Timer>(timer);
}

void TimerQueue::cancel(TimerId timerId) {
    auto timer = timerId.timer_.lock();
    if (timer) {
        loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timer));
    }
}

void TimerQueue::addTimerInLoop(std::shared_ptr<Timer> timer) {
    loop_->assertInLoopThread();
    Timer::TimeType expiredTime = timer->getExpiredTime(); // 调用 insert 函数，会移动 timer，所以先记录到期时间
    bool isEarliest = insert(timer); // 插入的定时器是否是最早到期的定时器
    if (isEarliest) {
        Timerfd::resetTimerfd(timerfd_, expiredTime); // 更新定时器文件描述符
    }
}

void TimerQueue::cancelInLoop(std::shared_ptr<Timer> timer) {
    loop_->isInLoopThread();
    Entry timerEntry(timer->getExpiredTime(), timer);
    auto it = timers_.find(timerEntry);
    if (it != timers_.end()) {
        size_t n = timers_.erase(timerEntry);
    } else if (callingExpiredTimers_) {
        /**
         * 防止定时器"自注销"，即定时器在自己的回调函数中注销自己。
         * callingExpiredTimers_ 为 true，即当前为处理到期定时器期间。
         * 此时，Timer 不在 timers_ 集合中，而是在 expired 集合中，且正在处理（即调用回调函数，运行到这里）。
         * 记录在处理到期定时器期间，有哪些定时器请求"自注销"（cancel）。
         * 处理完定时器后，更新定时器（reset）时，不再把"自注销"的定时器添加到 timers_ 中。
         * 等到下次处理到期定时器时，清理该集合，释放定时器资源。
         */
        cancelingTimers_.insert(timerEntry);
    }
}


void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timer::TimeType now = Timer::now();
    Timerfd::readTimerfd(timerfd_, now);
    std::vector<Entry> expiredTimers = getExpired(now);

    callingExpiredTimers_ = true;
    // TODO "自注销"定时器在此处被释放
    cancelingTimers_.clear(); // 清理上次处理定时器期间的"自注销"
    for (const auto &timer : expiredTimers) {
        timer.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expiredTimers, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timer::TimeType now) {
    std::vector<Entry> expired;
    // UINTPTR_MAX 是 uintptr_t 的最大值，使用最大值，避免冲突
    Entry sentry = std::make_pair(now, std::shared_ptr<Timer>(reinterpret_cast<Timer*>(UINTPTR_MAX)));
    auto it = timers_.lower_bound(sentry); // 找到第一个未到期 Timer 的迭代器

    assert(it == timers_.end() || now < sentry.first);
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    // TODO 调试的时候，确定一下 expired（shared_ptr） 有没有释放资源，即没有 reset 成功的定时器有没有释放
    // ROV优化，此处不会拷贝 vector
    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timer::TimeType now) {
    for (const auto &entry : expired) {
        Entry timer(entry.second->getExpiredTime(), entry.second);
        // 更新周期执行且不是"自注销"的定时器的到期时间，并且重新插入到定时器集合中
        if (entry.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            entry.second->restart(now);
            insert(entry.second);
        }
    }

    // 更新 timerfd 到期时间
    Timer::TimeType newExpiredTime;
    if (!timers_.empty() ) {
        auto earliestTimer = timers_.begin()->second;
        newExpiredTime = timers_.begin()->second->getExpiredTime();
        if (earliestTimer->isVaild()) {
            Timerfd::resetTimerfd(timerfd_, newExpiredTime);
        }
    }

}

bool TimerQueue::insert(std::shared_ptr<Timer> timer) {
    loop_->assertInLoopThread();
    bool isEarliest = false; // 定时器插入后是否为最早到期的定时器
    auto it = timers_.begin();
    // timers_ 没有定时器或者 timer 的到期时间比第一个定时器（timers_ 中最早到期的定时器）的到期时间早
    if (it == timers_.end() || timer->getExpiredTime() < it->first) {
        isEarliest = true;
    }
    /**
     * std::pair<iterator,bool> insert( const value_type& value );
     * 使用该版本的 insert 函数
     * T1 为插入后的迭代器，T2 为是否插入成功
     */
    std::pair<TimerList::iterator, bool> result
        = timers_.insert(std::make_pair(timer->getExpiredTime(), timer));
    assert(result.second);

    return isEarliest;
}
