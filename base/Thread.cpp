#include "Thread.h"

#include <cassert>
#include <cerrno>
#include <pthread.h>
#include <unistd.h>
#include <syscall.h>

#include <iostream>
#include <exception>

#include "Logger.h"

using namespace tinyWS;

Thread::Thread(const Thread::ThreadFunction &func, const std::string &name)
    : started_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(name) {
}

Thread::~Thread() {
    // no join
}

void Thread::start() {
    assert(!started_);

    // 启动线程
    started_ = true;
    errno = pthread_create(&pthreadId_, nullptr, &startThread, this);
    if (errno != 0) {
        debug(LogLevel::ERROR) << "Failed in pthread_create" << std::endl;
    }
}

bool Thread::started() {
    return started_;
}

int Thread::join() {
    assert(started_);

    return pthread_join(pthreadId_, nullptr);
}

pid_t Thread::tid() const {
    return tid_;
}

const std::string& Thread::name() const {
    return name_;
}


pid_t Thread::gettid() {
    return static_cast<pid_t>(::syscall(__NR_gettid));
}

void* Thread::startThread(void *obj) {
    auto *thread = static_cast<Thread*>(obj);
    thread->runInThread();
    return nullptr;
}

void Thread::runInThread() {
    tid_ = Thread::gettid();
    try {
        func_();
    } catch (const std::exception &ex) {
        fprintf(stderr, "exception caught in Thread %d\n", tid_);
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    } catch (...) {
        throw; // rethrow
    }
}
