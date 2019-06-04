//
// Created by 陈光明 on 2019-04-05.
//


#include "ThreadPool.h"

#include <cerrno>

#include <iostream>
#include <cassert>
#include <functional>
#include <algorithm>

using namespace std::placeholders;
using namespace tinyWS;

ThreadPool::ThreadPool(const std::string &name)
    : mutex_(),
      cond_(mutex_),
      name_(name),
      running_(false) {
}

ThreadPool::~ThreadPool() {
    if (running_) {
        stop();
    }
}

void ThreadPool::start(int numThreads) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(static_cast<ThreadPool::ThreadList::size_type >(numThreads));
    for (int i = 0; i < numThreads; ++i) {
        char id[32];
        snprintf(id, sizeof(id), "%d", i);
        threads_.push_back(std::unique_ptr<Thread>(new Thread(std::bind(&ThreadPool::runInThread, this), name_ + id)));
        threads_[i]->start();
    }
}

void ThreadPool::stop() {
    {

        MutexLockGuard lock(mutex_);
        running_ = false;
        cond_.notifyAll(); // 可移出临界区
    }
    // for_each 向 bind包装好的函数传递 Thread unique_ptr，因为join函数无形参，所以该实参作为 this指针传入
    std::for_each(threads_.begin(), threads_.end(), std::bind(&Thread::join, _1));
}

void ThreadPool::run(const ThreadPool::Task &task) {
    if (threads_.empty()) {
        task();
    } else {
        MutexLockGuard lock(mutex_);
        dequeue_.push_back(task);
        cond_.notify();
    }
}

void ThreadPool::runInThread() {
    try {
        while (running_) {
            Task task(take());
            if (task) {
                task();
            }
        }
    } catch (const std::exception &ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    } catch (...) {
        fprintf(stderr, "unkonw exception caugt in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }

}

ThreadPool::Task ThreadPool::take() {
    MutexLockGuard lock(mutex_);
    // 忙等待，以防虚假唤醒
    while (dequeue_.empty() && running_) {
        cond_.wait();
    }

    Task task;
    if (!dequeue_.empty()) {
        task = dequeue_.front();
        dequeue_.pop_front();
    }
    return task;
}
