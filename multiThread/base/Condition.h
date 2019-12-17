#ifndef TINYWS_CONDITION_H
#define TINYWS_CONDITION_H

#include <pthread.h>
#include <cassert>
#include <cerrno>

#include "MutexLock.h"
#include "noncopyable.h"

namespace tinyWS_thread {
    // 条件变量对象
    class Condition : noncopyable {
    public:
        explicit Condition(MutexLock &mutex)
            : mutex_(mutex),
              cond_{} {
            assert(pthread_cond_init(&cond_, nullptr) == 0);
        }

        ~Condition() {
            assert(pthread_cond_destroy(&cond_) == 0);
        }

        void wait() {
            pthread_cond_wait(&cond_, mutex_.getPthreadMutexPtr());
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
            return pthread_cond_timedwait(&cond_, mutex_.getPthreadMutexPtr(), &timeout) == ETIMEDOUT;
        }

        void notify() {
            pthread_cond_signal(&cond_);
        }

        void notifyAll() {
            pthread_cond_broadcast(&cond_);
        }

    private:
        MutexLock &mutex_;
        pthread_cond_t cond_;
    };
}


#endif //TINYWS_CONDITION_H
