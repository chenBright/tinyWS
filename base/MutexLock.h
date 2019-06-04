#ifndef TINYWS_MUTEXLOCK_H
#define TINYWS_MUTEXLOCK_H

#include <pthread.h>
#include <cassert>

#include "Thread.h"
#include "noncopyable.h"

namespace tinyWS {
    /**
     * TODO
     * 这段代码没有达到工业强度，详见书 P46
     */

    class MutexLock : noncopyable {
    public:
        MutexLock() : holder_(0) {
            assert(pthread_mutex_init(&mutex_, nullptr) == 0);
        }

        ~MutexLock() {
            assert(holder_ == 0);
            assert(pthread_mutex_destroy(&mutex_) == 0);
        }

        bool isLockedByThisThread() {
            return holder_ == Thread::gettid();
        }

        void assertLocked() {
            assert(isLockedByThisThread());
        }

        /**
         * 加锁（仅供 MutexLockGuard 调用，严禁用户调用）
         */
        void lock() {
            pthread_mutex_lock(&mutex_); // 必须先上锁，才能修改holder_
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
        pthread_mutex_t mutex_;
        pid_t holder_;
    };

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
