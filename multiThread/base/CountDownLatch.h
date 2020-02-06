#ifndef TINYWS_COUNTDOWNLATCH_H
#define TINYWS_COUNTDOWNLATCH_H

#include "noncopyable.h"
#include "MutexLock.h"
#include "Condition.h"

namespace tinyWS_thread {

    class CountDownLatch : noncopyable {
    private:
        mutable MutexLock mutex_;
        Condition condition_;
        int count_;

    public:
        explicit CountDownLatch(int count);

        void wait();

        void countDown();

        int getCount() const;
    };
}

#endif //TINYWS_COUNTDOWNLATCH_H
