#ifndef TINYWS_BLOCKINGQUEUE_H
#define TINYWS_BLOCKINGQUEUE_H

#include <cassert>

#include <deque>

#include "noncopyable.h"
#include "MutexLock.h"
#include "Condition.h"

namespace tinyWS_thread {

    template <class T>
    class BlockingQueue : noncopyable {
    public:
        BlockingQueue() : mutex_(), notEmpty_(mutex_), queue_() {}

        void put(const T &value) {
            MutexLockGuard lock(mutex_);
            queue_.push_back(value);
            notEmpty_.notify();
        }

        void put(T &&value) {
            MutexLockGuard lock(mutex_);
            queue_.push_back(std::move(value));
            notEmpty_.notify();
        }

        T take() {
            MutexLockGuard lock(mutex_);
            while (queue_.empty()) {
                notEmpty_.wait();
            }
            assert(!queue_.empty());

            T front(std::move(queue_.front()));
            queue_.pop_front();

            return front;
        }

        size_t size() const {
            MutexLockGuard lock(mutex_);
            return queue_.size();
        }

        bool empty() const {
            return size() == 0;
        }

    private:
        mutable MutexLock mutex_;
        Condition notEmpty_;
        std::deque<T> queue_;
    };
}


#endif //TINYWS_BLOCKINGQUEUE_H
