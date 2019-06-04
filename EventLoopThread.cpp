#include "EventLoopThread.h"

#include <cassert>

#include "EventLoop.h"

using namespace tinyWS;

EventLoopThread::EventLoopThread(const EventLoopThreadCallback &cb)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunction, this)),
      mutex_(),
      cond_(mutex_),
      callback_(cb) {

}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startThread() {
    assert(!thread_.started());
    thread_.start();

    {
        MutexLockGuard lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait();
        }
    }
    return loop_;
}

/**
 * 在新创建的线程 stack 上定义 EventLoop 对象，然后将其地址赋值给 loop_ 成员变量
 */
void EventLoopThread::threadFunction() {
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
}