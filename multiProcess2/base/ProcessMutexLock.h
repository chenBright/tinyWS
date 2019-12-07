#ifndef TINYWS_PROCESSMUTEXLOCK_H
#define TINYWS_PROCESSMUTEXLOCK_H

#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

#include "noncopyable.h"


namespace tinyWS_process2 {

    class ProcessMutexLock : noncopyable {
    private:
        pthread_mutex_t* mutex_;
    public:
        ProcessMutexLock() : mutex_(nullptr) {
            int fd = open("/dev/zero", O_RDWR, 0);

            pthread_mutexattr_t mutexattr{};
            mutex_ = static_cast<pthread_mutex_t*>(mmap(nullptr, sizeof(pthread_mutexattr_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
            close(fd);

            pthread_mutexattr_init(&mutexattr);
            pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(mutex_, &mutexattr);
        }

        ~ProcessMutexLock() {
            // destory ?
            pthread_mutex_destroy(mutex_);
        }

        void lock() {
            pthread_mutex_lock(mutex_);
        }

        bool trylock() {
            int result = pthread_mutex_trylock(mutex_);
//            std::cout << result << std::endl;
            return  result == 0;
        }

        void unlock() {
            pthread_mutex_unlock(mutex_);
        }
    };

    // 自动维护互斥量加锁和解锁：
    // 1 创建对象的时候，加锁；
    // 2 离开作用域，销毁对象时，解锁
    class ProcessMutexLockGuard : noncopyable {
    public:
        explicit ProcessMutexLockGuard(ProcessMutexLock &mutex) : mutex_(mutex) {
            mutex_.lock();
        }

        ~ProcessMutexLockGuard() {
            mutex_.unlock();
        }

    private:
        ProcessMutexLock &mutex_;
    };

    /**
     * 防止类似误用：ProcessMutexLockGuard(mutex_)
     * 临时对象不能长时间持有锁，一产生对象又马上被销毁！
     * 正确写法：ProcessMutexLockGuard lock(mutex)
     */
    #define ProcessMutexLockGuard(x) error "Missing guard object name"
}


#endif //TINYWS_PROCESSMUTEXLOCK_H
