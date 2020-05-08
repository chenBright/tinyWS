#ifndef TINYWS_SPINLOCK_H
#define TINYWS_SPINLOCK_H


#include <pthread.h>
#include <cassert>

#include "Thread.h"
#include "noncopyable.h"

namespace tinyWS_thread {

    // 参考
    // https://www.cnblogs.com/chris-cp/p/5413445.html
    class SpinLock : noncopyable {
    private:
        pthread_spinlock_t spinlock_;   // 自旋锁
        pid_t holder_;                  // 持有锁的线程的 ID

    public:
        SpinLock() : spinlock_{}, holder_(0) {
            // PTHREAD_PROCESS_SHARED：该自旋锁可以在多个进程中的线程之间共享。
            // PTHREAD_PROCESS_PRIVATE: 仅初始化本自旋锁的线程所在的进程内的线程才能够使用该自旋锁。
            assert(pthread_spin_init(&spinlock_, PTHREAD_PROCESS_PRIVATE) == 0);
        }

        ~SpinLock() {
            assert(holder_ == 0);
            assert(pthread_spin_destroy(&spinlock_));
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

        void lock() {
            pthread_spin_lock(&spinlock_);  // 必须先加锁，才能修改holder_
            holder_ = Thread::gettid();
        }

        void unlock() {
            holder_ = 0;
            pthread_spin_unlock(&spinlock_);
        }

        pthread_spinlock_t* getPthreadSpinLock() {
            return &spinlock_;
        }
    };

    // 自动维护自旋锁加锁和解锁：
    // 1 创建对象的时候，加锁；
    // 2 离开作用域，销毁对象时，解锁
    class SpinLockGuard : noncopyable {
    private:
        SpinLock& spinlock_;
    public:
        explicit SpinLockGuard(SpinLock& spinlock) : spinlock_(spinlock) {
            spinlock_.lock();
        }

        ~SpinLockGuard() {
            spinlock_.unlock();
        }
    };

    // 防止类似误用：SpinLockGuard(spinlock_)
    // 临时对象不能长时间持有锁，一产生对象又马上被销毁！
    // 正确写法：SpinLockGuard lock(spinlock_)
    #define MutexLockGuard(x) error "Missing guard object name"
}


#endif //TINYWS_SPINLOCK_H
