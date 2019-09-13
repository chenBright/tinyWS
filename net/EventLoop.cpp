#include "EventLoop.h"

#include <cassert>
#include <sys/eventfd.h>
#include <unistd.h>

#include <iostream>
#include <functional>

#include "../base/Logger.h"
#include "../base/Thread.h"
#include "Channel.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "TimerId.h"

using namespace tinyWS;

// __thread 关键字可将变量声明为线程局部变量
__thread EventLoop *t_loopInThisThread = nullptr; // IO 线程

const int kEpollTimeMs = 10000;

int createEventfd() {
    // TODO 学习 eventfd
    int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0) {
        debug(LogLevel::ERROR) << "Failed in eventfd" << std::endl;
        abort();
    }

    return evfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFuntors_(false),
      threadId_(Thread::gettid()),
      epoll_(new Epoll(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {

    debug() << "EventLoop created "
            << this << " in thread "
            << threadId_ << std::endl;

    if (t_loopInThisThread) { // 已创建 EventLoop
        debug(LogLevel::TRACE) << "Another EventLoop " << t_loopInThisThread
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
    assert(!looping_); // 确保 EventLoop 对象析构的时候，已经退出事件循环
    debug() << "EventLoop::~EventLoop destructing" << std::endl;

    // 清除 wakeupfd_ 相关资源
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);

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
    // 未启动事件循环，且该函数在 IO 线程中调用
    assert(!looping_);
    assertInLoopThread();

    looping_ = true;
    quit_ = false;

    debug() << "EventLoop " << this << "start looping" << std::endl;

    while (!quit_) {
        activeChannels_.clear(); // 清空 Channel 列表，以获取新的 Channel 列表
        auto receiveTime = epoll_->poll(kEpollTimeMs, &activeChannels_); // 阻塞，等待事件的"到来"
        for (const auto &channel : activeChannels_) {
            channel->handleEvent(receiveTime);
        }
        doPendingFunctors();
    }

    debug() << "EVentLoop " << this << " stop looping" << std::endl;
    looping_ = false;
}

void EventLoop::quit() {
    // EventLoop::quit() 不会让事件循环立即退出，
    // EventLoop::loop() 中事件循环会在下一次检查 while(!quit_) 时退出
    quit_ = true;

    // 如果调用线程不是 IO 线程，则唤醒 IO 线程，让其退出事件循环；
    // 如果调用线程是 IO 线程，则表示当前在处理 pendingFunctors
    //（在 EventLoop::doPendingFunctors() 处理），EventLoop::quit() 是其中一个 pendingFunctor。
    //当处理完后，会进行下一次 while(!quit_) 的检查。此时，事件循环会退出。
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor &cb) {
    // 如果在 IO 线程中调用该函数，则直接调用回调函数；
    // 否则，加入队列中。
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

    // 有两种情况需要唤醒 IO 线程：
    // 1. 在非 IO 线程调用 EventLoop::queueInLoop()；
    // 2. 在 IO 线程调用 EventLoop::queueInLoop()，但此时正在调用 EventLoop::doPendingFunctors()。
    //    实质上，EventLoop::queueInLoop() 就是在其中一个 pending functor 中被调用了。
    //    根据 EventLoop::doPendingFunctors() 的实现，doPendingFunctors() 函数被调用时，
    //    为了缩小临界区，会将 pendingFunctors_ 数组 swap 到一个临时数组中，再处理。
    //    那此时加入队列的函数是不会在此次处理 pending functor 中被调用。
    //    所以，为了尽快调用该 cb 函数，则需要调用 EventLoop::wakeup()，在下次循环中立即唤醒 IO 线程。
    //
    // 总结，只有在 IO 线程的事件回调函数中调用 EventLoop::queueInLoop()
    // 才无须调用 EventLoop::wakeup() 唤醒 IO 线程。因为在事件回调处理完之后，
    // 会调用 doPendingFunctors() 函数，处理 pending functor，该 cb  函数也会被调用。
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

void EventLoop::cancle(const TimerId &timerId) {
    timerQueue_->cancel(timerId);
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    // 往 wakeupfd_ 写入一个字节的数据，唤醒 IO 线程
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        debug(LogLevel::TRACE) << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
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

bool EventLoop::hasChannel(tinyWS::Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return epoll_->hasChannel(channel);
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
    debug(LogLevel::ERROR) << "Error: EventLoop::abortNotInLoopThread - EventLoop " << this
                                 << " was created in threadId_ = " << threadId_
                                 << ", current thread id = " <<  Thread::gettid() << std::endl;
}


void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(n)) {
        debug(LogLevel::TRACE) << "EventLoop::handleRead() reads "
                                     << n << " bytes instead of 8"
                                     << std::endl;
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFuntors_ = true;

    // 将 pendingFunctors_ swap 到临时 functors，再处理 pending functor，
    // 这样可以缩短临界区，同时清空 pendingFunctors_。
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
        debug() << "{" << channel->reventsToString() << "} " << std::endl;
    }
}