#include "TimerQueue.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <cassert>

#include <functional>
#include <algorithm>
#include <utility>

#include "../base/Logger.h"
#include "EventLoop.h"

using namespace tinyWS_thread;

// timerfd 操作函数集合
namespace Timerfd {
    int createTimerfd() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0) {
            debug(LogLevel::ERROR) << "Failed in timerfd_create" << std::endl;
        }

        return timerfd;
    }

    /**
     * 读取 timerfd 的数据
     * @param timerfd
     * @param now 读取数据的时间
     */
    void readTimerfd(int timerfd, Timer::TimeType now) {
        uint64_t howmany;
        ssize_t n = read(timerfd, &howmany, sizeof(howmany));
        debug(LogLevel::ERROR) << "TimerQueue::handleRead() "
                                     << howmany << " at " << now << std::endl;
        if (n != sizeof(howmany)) {
            debug(LogLevel::ERROR) << "TimerQueue::handleRead() reads "
                                         << n << " bytes instead of 8" << std::endl;
        }
    }

    /**
     * 将时间转换为 timespec 格式
     * @param when 某一时刻
     * @return timespec 格式的时间
     */
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

   /**
    * 更新定时器，唤醒 IO 线程，处理定时器任务
    * @param timerfd
    * @param when 时间
    */
    void resetTimerfd(int timerfd, Timer::TimeType when) {
        itimerspec oldValue{};
        itimerspec newValue{};
        newValue.it_value = howMuchTimeFromNow(when);
        // 通过 timerfd_settime 函数唤醒 IO 线程
        int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if (ret) {
            debug(LogLevel::ERROR) << "timerfd_settime()" << std::endl;
        }
    }
};

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(Timerfd::createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      callingExpiredTimers_(false) {

    // 设置"读"回调函数和可读
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    close(timerfd_);
    // 直到 EventLoop 析构之前，都不能移除 channel
}

TimerId TimerQueue::addTimer(const Timer::TimerCallback &cb, Timer::TimeType timeout, Timer::TimeType interval) {
    std::shared_ptr<Timer> timer(new Timer(cb, timeout, interval));
    // 将添加定时器的实际工作转移到 IO 线程，使得不加锁也能保证线程安全性
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));

    return *new TimerId(std::weak_ptr<Timer>(timer));
}

void TimerQueue::cancel(const TimerId &timerId) {
    // TimerQueue 是 TimerId 的友元类
    auto timer = timerId.timer_.lock();
    if (timer) {
        // 将注销定时器的实际工作转移到 IO 线程，使得不加锁也能保证线程安全性
        loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
    }
}

void TimerQueue::addTimerInLoop(const std::shared_ptr<Timer> &timer) {
    loop_->assertInLoopThread();
    Timer::TimeType expiredTime = timer->getExpiredTime();
    bool isEarliest = insert(timer); // 插入的定时器是否是最早到期的定时器
    if (isEarliest) {
        Timerfd::resetTimerfd(timerfd_, expiredTime); // 更新定时器文件描述符
    }
}

void TimerQueue::cancelInLoop(const TimerId &timerId) {
    loop_->isInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    auto cancelTimer = timerId.timer_.lock();
    assert(cancelTimer);
    ActiveTimer timer(cancelTimer, cancelTimer->getSequence());
    auto it = activeTimers_.find(timer);
    if (it != activeTimers_.end()) {
        // 删除两个容器中的定时器
        auto n = timers_.erase(Entry(it->first->getExpiredTime(), it->first));
        assert(n == 1);
        (void)(n);
        activeTimers_.erase(it);
    } else if (callingExpiredTimers_) {
        // 防止定时器"自注销"，即定时器在自己的回调函数中注销自己。
        // callingExpiredTimers_ 为 true，即当前为处理到期定时器期间。
        // 此时，Timer 不在 timers_ 集合中，而是在 expired 集合中，且正在处理（即调用回调函数，运行到这里）。
        // 记录在处理到期定时器期间，有哪些定时器请求"自注销"（cancel）。
        // 处理完定时器后，更新定时器（reset）时，不再把"自注销"的定时器添加到 timers_ 中。
        // 等到下次处理到期定时器时，清理该集合，释放定时器资源。
        cancelingTimers_.insert(timer);
    }

    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timer::TimeType now = Timer::now();
    Timerfd::readTimerfd(timerfd_, now); // 读取数据，防止重复触发可读事件
    std::vector<Entry> expiredTimers = getExpired(now); // 读取到期定时器

    callingExpiredTimers_ = true; // 标记正在处理定时器
    cancelingTimers_.clear(); // 清理上次处理定时器期间的"自注销"定时器
    for (const auto &timer : expiredTimers) {
        timer.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expiredTimers, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timer::TimeType now) {
    assert(timers_.size() == activeTimers_.size());

    // UINTPTR_MAX 是 uintptr_t 的最大值，使用最大值，避免冲突
    Entry sentry = std::make_pair(now, std::shared_ptr<Timer>(reinterpret_cast<Timer*>(UINTPTR_MAX)));
    auto it = timers_.lower_bound(sentry); // 找到第一个未到期 Timer 的迭代器

    assert(it == timers_.end() || now < it->first);

    std::vector<Entry> expired;
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    // 从 activeTimers_ 中删除过期定时器
    for (const auto &activated : expired) {
        ActiveTimer timer(activated.second, activated.second->getSequence());
        auto n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)(n);
    }

    assert(timers_.size() == activeTimers_.size());

    // ROV优化，此处不会拷贝 vector
    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timer::TimeType now) {
    for (const auto &entry : expired) {
        ActiveTimer timer(entry.second, entry.second->getSequence());
        // 更新周期执行且不是"自注销"定时器的到期时间，并且重新插入到定时器集合中
        if (entry.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            entry.second->restart(now);
            insert(entry.second);
        }
    }

    // 更新 timerfd 到期时间
    if (!timers_.empty() ) {
        auto earliestTimer = timers_.begin()->second;
        if (earliestTimer->isValid()) {
            auto newExpiredTime = earliestTimer->getExpiredTime();
            Timerfd::resetTimerfd(timerfd_, newExpiredTime);
        }
    }

}

bool TimerQueue::insert(const std::shared_ptr<Timer>& timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    bool isEarliest = false; // 定时器插入后是否为最早到期的定时器
    auto it = timers_.begin();
    // timers_ 没有定时器或者 timer 的到期时间比第一个定时器（timers_ 中最早到期的定时器）的到期时间早
    if (it == timers_.end() || timer->getExpiredTime() < it->first) {
        isEarliest = true;
    }

    // 将定时器插入到两个容器中
    {
        // std::pair<iterator,bool> insert( const value_type& value );
        // 使用该版本的 insert 函数
        // T1 为插入后的迭代器，T2 为是否插入成功
        std::pair<TimerList::iterator, bool> result
            = timers_.insert(std::make_pair(timer->getExpiredTime(), timer));
        assert(result.second);
        (void)result;
    }

    {
        // 同上
        std::pair<ActiveTimerSet::iterator, bool> result
                = activeTimers_.insert(std::make_pair(timer, timer->getSequence()));
        assert(result.second);
        (void)result;
    }

    assert(timers_.size() == activeTimers_.size());

    return isEarliest;
}
