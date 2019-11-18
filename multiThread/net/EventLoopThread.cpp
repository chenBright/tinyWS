#include "EventLoopThread.h"

#include <cassert>

#include "EventLoop.h"

using namespace tinyWS_thread;

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
    // 如果 IO 线程已经创建，
    // 则要退出 EventLoop 和 结束线程。
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startThread() {
    // 检查线程是否启动，防止重复启动线程
    assert(!thread_.started());
    thread_.start();

    {
        // 使用条件变量，一直等到 IO 线程创建好，
        // 即 loop_ 不为 nullptr，再返回 loop_。
        MutexLockGuard lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait();
        }
    }
    return loop_;
}

/**
 * 在新创建的线程 stack 上定义 EventLoop 对象，
 * 然后将其地址赋值给 loop_ 成员变量，
 * 最后，使用条件变量唤醒主线程。
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