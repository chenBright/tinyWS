#include "EventLoopThreadPool.h"

#include <cassert>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "base/MutexLock.h"

using namespace tinyWS;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop)
    : baseLoop_(baseLoop),
      started_(false),
      numThreads_(0),
      next_(0) {

}

EventLoopThreadPool::~EventLoopThreadPool() {
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::setThreadNum(int numThreads) {
    assert(numThreads >= 0);
    numThreads_ = numThreads;
}

void EventLoopThreadPool::start(const EventLoopThreadPool::EventLoopThreadPoolCallback &cb) {
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        EventLoopThread *t = new EventLoopThread(cb);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startThread());
    }
    // 如果线程池为空，则在主线程执行
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    // 如果线程池为空，则返回主线程
    EventLoop *loop = baseLoop_;

    if (!loops_.empty()) {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (static_cast<decltype(loops_.size())>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }

    return loop;
}