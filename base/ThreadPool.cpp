#include "ThreadPool.h"

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
    // 先分配足够的空间来存储线程对象，避免频繁内存空间重分配
    threads_.reserve(static_cast<ThreadPool::ThreadList::size_type>(numThreads));
    for (int i = 0; i < numThreads; ++i) {
        char id[32];
        snprintf(id, sizeof(id), "%d", i);
        // 每个线程都会调用 ThreadPool::runInThread() 函数，
        // 该函数会不断地从任务队列中取出任务执行。
        threads_.push_back(std::unique_ptr<Thread>(
                           new Thread(std::bind(&ThreadPool::runInThread, this),
                           name_ + id)));
        threads_[i]->start();
    }
}

void ThreadPool::stop() {
    {

        MutexLockGuard lock(mutex_);
        running_ = false;
        cond_.notifyAll(); // 可移出临界区
    }
    // for_each 向 bind 包装好的函数传递 Thread unique_ptr，作为 this 指针传入
    std::for_each(threads_.begin(), threads_.end(), std::bind(&Thread::join, _1));
}

void ThreadPool::run(const ThreadPool::Task &task) {
    if (threads_.empty()) {
        // 如果线程池为空，只有主线程，则直接在主线程中执行任务。
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
