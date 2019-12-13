#ifndef TINYWS_PROCESSCONDITION_H
#define TINYWS_PROCESSCONDITION_H

#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cassert>

#include "noncopyable.h"
#include "ProcessMutexLock.h"

namespace tinyWS_process2 {

    class ProcessCondition : noncopyable {
    private:
        ProcessMutexLock &mutex_;
        pthread_cond_t* cond_;

    public:
        explicit ProcessCondition(ProcessMutexLock& mutex)
            : mutex_(mutex),
              cond_{} {

            // 读设备 /dev/zero 时，该设备是 0 字节的无限资源。
            // 它可以接受写向它的任何数据，但又忽略这些数据。
            int fd = open("/dev/zero", O_RDWR, 0);

            // PROT_READ | PROT_WRITE：设置映射区域可读可写。
            // MAP_SHARED：父进程在调用 mmap 函数时指定了 MAP_SHARED 标志，则这些进程可共享此内存区域。从而达到了共享内存的目的。
            cond_ = static_cast<pthread_cond_t*>(mmap(nullptr,
                                                       sizeof(pthread_cond_t),
                                                       PROT_READ | PROT_WRITE,
                                                       MAP_SHARED, fd,
                                                       0));
            close(fd); // 已经映射完成，可以关闭文件描述符。

            pthread_condattr_t condattr{};
            pthread_condattr_init(&condattr);
            // PTHREAD_PROCESS_SHARED：允许在不同进程之间共享条件变量。
            pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
            assert(pthread_cond_init(cond_, nullptr) == 0);
        }

        ~ProcessCondition() {
            // 销毁条件变量，释放映射的内存区域
            pthread_cond_destroy(cond_);
            munmap(cond_, sizeof(pthread_cond_t));
        }

        void wait() {
            pthread_cond_wait(cond_, mutex_.getPthreadMutexPtr());
        }

        /**
         * 等待规定时间
         * @param second 等待的时间
         * @return 如果超时，则返回true；否则，返回false
         */
        bool waitForSecond(int second) {
            struct timespec timeout{};
            clock_getres(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += second;
            return pthread_cond_timedwait(cond_, mutex_.getPthreadMutexPtr(), &timeout) == ETIMEDOUT;
        }

        void notify() {
            pthread_cond_signal(cond_);
        }

        void notifyAll() {
            pthread_cond_broadcast(cond_);
        }
    };

}

#endif //TINYWS_PROCESSCONDITION_H
