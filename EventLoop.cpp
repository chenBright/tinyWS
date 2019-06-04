#include "EventLoop.h"

#include <cassert>
#include <sys/eventfd.h>
#include <unistd.h>

#include <iostream>
#include <functional>

#include "base/Thread.h"
#include "Channel.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "TimerId.h"

using namespace tinyWS;

// __thread 关键字可将变量声明为 线程局部变量
__thread EventLoop *t_loopInThisThread = nullptr; // EventLoop 所属线程

const int kEpollTimeMs = 10000;

int createEventfd() {
    // TODO 学习 eventfd
    int evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0) {
        std::cout << "Failed in eventfd" << std::endl;
        abort();
    }

    return evfd;
}

EventLoop::EventLoop()
    : looping_(false),
      callingPendingFuntors_(false),
      threadId_(Thread::gettid()),
      epoll_(new Epoll(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {

    std::cout << "EventLoop created "
        << this << " in thread "
        << threadId_ << std::endl;

    if (t_loopInThisThread) { // 已创建 EventLoop
        std::cout << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_ << std::endl;
    } else {
        t_loopInThisThread = this;
    }

    // 设置 wakeupfd Channel
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // wakeupfd 一直可读
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    assert(!looping_);
    t_loopInThisThread = nullptr;
}

void EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) {
        abortNotInLoopThread();
    }
}

bool EventLoop::isInLoopThread() const {
    return threadId_ == Thread::gettid();
}

void EventLoop::loop() {
    // 未启动时间循环，且在创建线程中
    assert(!looping_);
    assertInLoopThread();

    looping_ = true;
    quit_ = false;

    while (!quit_) {
        activeChannels_.clear();
        epoll_->poll(kEpollTimeMs, &activeChannels_); // 阻塞
        for (const auto &channel : activeChannels_) {
            channel->handleEvent(0);
        }
        doPendingFunctors();
    }

    std::cout << "EVentLoop " << this << " stop looping" << std::endl;
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor &cb) {
    /**
     * 如果在 EventLoop 线程中，则直接调用回调函数；
     * 否则，加入队列中。
     */
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    if (!isInLoopThread() || callingPendingFuntors_) {
        wakeup();
    }
}

TimerId EventLoop::runAt(Timer::TimeType time, const Timer::TimeCallback &cb) {
    return timerQueue_->addTimer(cb, time, 0);
}

TimerId EventLoop::runAfter(Timer::TimeType delay, const Timer::TimeCallback &cb) {
    return runAt(Timer::now() + delay, cb);
}

TimerId EventLoop::runEvery(Timer::TimeType interval, const Timer::TimeCallback &cb) {
    return timerQueue_->addTimer(cb, Timer::now(), interval);
}

void EventLoop::cancle(TimerId timerId) {
    timerQueue_->cancel(timerId);
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
    }
}

void EventLoop::updateChannel(Channel *channel) {
    // 确保 channel 属于该事件循环，且在创建线程内调用该函数
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    epoll_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    epoll_->removeChannel(channel);
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
    std::cout << "Error: EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  Thread::gettid() << std::endl;
}


void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(n)) {
        std::cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8" << std::endl;
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFuntors_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const auto &functor : functors) {
        functor();
    }
    callingPendingFuntors_ = false;
}

void EventLoop::printActiveChannels() const {
    for (const auto &channel : activeChannels_) {
//        std::cout << "{" << channel->reventsToString() << "} " << std::endl;
    }
}