#include "ThreadPool_cpp11.h"

#include <cstdint>

#include <cassert>
#include <cstdio>
#include <functional>
#include <algorithm>

#include "Thread.h"
#include "Logger.h"

using namespace std::placeholders;
using namespace tinyWS_thread;

ThreadPool_cpp11::ThreadPool_cpp11(const std::string& name)
    : name_(name),
      running_(false) {
}

ThreadPool_cpp11::~ThreadPool_cpp11() {
    if (running_) {
        stop();
    }
}

void ThreadPool_cpp11::start(int numThreads) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(static_cast<ThreadList::size_type>(numThreads));
    for (int i = 0; i < numThreads; ++i) {
        char id[32];
        snprintf(id, sizeof(id), "%d", i);
        // 每个线程都会调用 ThreadPool::runInThread() 函数，
        // 该函数会不断地从任务队列中取出任务执行。
        threads_.emplace_back(std::bind(&ThreadPool_cpp11::runInThread, this));
    }
}

void ThreadPool_cpp11::stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
        cond_.notify_all();
    }
    // for_each 向 bind 包装好的函数传递 Thread unique_ptr，作为 this 指针传入
    std::for_each(threads_.begin(), threads_.end(), [](std::thread& t) {
        t.join();
    });
}


void ThreadPool_cpp11::run(const Task& task) {
    if (threads_.empty()) {
        // 如果线程池为空，只有主线程，则直接在主线程中执行任务。
        task();
    } else {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(task);
        cond_.notify_one();
    }
}

void ThreadPool_cpp11::runInThread() {
    try {
        while (running_) {
            Task task(take());
            if (task) {
                task();
            }
        }
    } catch (const std::exception& ex) {
        debug(LogLevel::ERROR) << "exception caught in ThreadPool " << name_.c_str() << std::endl;
        debug(LogLevel::ERROR) << "reason: " << ex.what() << std::endl;
        abort();
    } catch (...) {
        debug(LogLevel::ERROR) << "unkonw exception caugt in ThreadPool " << name_.c_str() << std::endl;
        throw; // rethrow
    }

}

ThreadPool_cpp11::Task ThreadPool_cpp11::take() {
    std::unique_lock<std::mutex> lock(mutex_);
    // 忙等待，以防虚假唤醒
    while (queue_.empty() && running_) {
        cond_.wait(lock);
    }

    Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop();
    }

    return task;
}
