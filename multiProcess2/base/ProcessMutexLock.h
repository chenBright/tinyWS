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
            // 读设备 /dev/zero 时，该设备是 0 字节的无限资源。
            // 它可以接受写向它的任何数据，但又忽略这些数据。
            int fd = open("/dev/zero", O_RDWR, 0);

            // PROT_READ | PROT_WRITE：设置映射区域可读可写。
            // MAP_SHARED：父进程在调用 mmap 函数时指定了 MAP_SHARED 标志，则这些进程可共享此内存区域。从而达到了共享内存的目的。
            mutex_ = static_cast<pthread_mutex_t*>(mmap(nullptr,
                                                         sizeof(pthread_mutex_t),
                                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                                         fd,
                                                         0));
            close(fd); // 已经映射完成，可以关闭文件描述符。

            pthread_mutexattr_t mutexattr{};
            pthread_mutexattr_init(&mutexattr);
            // PTHREAD_PROCESS_SHARED：允许在不同进程之间共享互斥量。
            pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(mutex_, &mutexattr);
        }

        ~ProcessMutexLock() {
            // 销毁互斥锁，释放映射的内存区域
            pthread_mutex_destroy(mutex_);
            munmap(mutex_, sizeof(pthread_mutexattr_t));
        }

        /**
         * 加锁
         */
        void lock() {
            pthread_mutex_lock(mutex_);
        }

        /**
         * 尝试加锁
         * @return 是否加锁成功
         */
        bool trylock() {
            int result = pthread_mutex_trylock(mutex_);

            return  result == 0;
        }

        /**
         * 释放锁
         */
        void unlock() {
            pthread_mutex_unlock(mutex_);
        }

        /**
         * 获取互斥量原始指针（仅供 Condition 调用，严禁用户调用）
         * 仅供 Condition 调用
         * @return 互斥量原始指针
         */
        pthread_mutex_t* getPthreadMutexPtr() {
            return mutex_;
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
