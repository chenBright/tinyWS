#ifndef TINYWS_MUTEXLOCK_H
#define TINYWS_MUTEXLOCK_H

#include <pthread.h>
#include <cassert>

#include "Thread.h"
#include "noncopyable.h"

namespace tinyWS {
    // 这段代码没有达到工业强度，详见 《Linux多线程服务端编程》P46
    class MutexLock : noncopyable {
    public:
        MutexLock() : holder_(0) {
            assert(pthread_mutex_init(&mutex_, nullptr) == 0);
        }

        ~MutexLock() {
            assert(holder_ == 0);
            assert(pthread_mutex_destroy(&mutex_) == 0);
        }

        /**
         * 当前线程是否为加锁线程
         * @return true / false
         */
        bool isLockedByThisThread() {
            return holder_ == Thread::gettid();
        }

        /**
         * 断言，当前线程是否为加锁线程
         */
        void assertLocked() {
            assert(isLockedByThisThread());
        }

        /**
         * 加锁（仅供 MutexLockGuard 调用，严禁用户调用）
         */
        void lock() {
            pthread_mutex_lock(&mutex_); // 必须先加锁，才能修改holder_
            holder_ = Thread::gettid();
        }

        /**
         * 释放锁（仅供 MutexLockGuard 调用，严禁用户调用）
         */
        void unlock() {
            holder_ = 0;
            pthread_mutex_unlock(&mutex_);
        }

        /**
         * 获取互斥量原始指针（仅供 Condition 调用，严禁用户调用）
         * 仅供 Condition 调用
         * @return 互斥量原始指针
         */
        pthread_mutex_t* getPthreadMutexPtr() {
            return &mutex_;
        }
    private:
        pthread_mutex_t mutex_{}; // 互斥量
        pid_t holder_;          // 存储持有锁的线程 ID
    };

    // 自动维护互斥量加锁和解锁：
    // 1 创建对象的时候，加锁；
    // 2 离开作用域，销毁对象时，解锁
    class MutexLockGuard : noncopyable {
    public:
        explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex) {
            mutex_.lock();
        }

        ~MutexLockGuard() {
            mutex_.unlock();
        }

    private:
        MutexLock &mutex_;
    };
}

/**
 * 防止类似误用：MutexLockGuard(mutex_)
 * 临时对象不能长时间持有锁，一产生对象又马上被销毁！
 * 正确写法：MutexLockGuard lock(mutex)
 */
#define MutexLockGuard(x) error "Missing guard object name"

#endif //TINYWS_MUTEXLOCK_H
