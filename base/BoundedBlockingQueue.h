#ifndef TINYWS_BOUNDEDBLOCKINGQUEUE_H
#define TINYWS_BOUNDEDBLOCKINGQUEUE_H

#include <cassert>

#include <deque>

#include "noncopyable.h"
#include "MutexLock.h"
#include "Condition.h"

namespace tinyWS {

    template<class T>
    class BoundedBlockingQueue {
    public:
        explicit BoundedBlockingQueue(size_t maxSize)
        : mutex_(),
          notEmpty_(mutex_),
          notFull_(mutex_),
          size_(0),
          maxSize_(maxSize),
          queue_(maxSize_) {}

        void put(const T &value) {
            MutexLockGuard lock(mutex_);
            // 一直等到到为队列不满，即有空位置
            while (size_ == maxSize_) {
                notEmpty_.wait();
            }
            assert(size_ < maxSize_);
            queue_.push_back(value);
            ++size_;
            notEmpty_.notify(); // 通知不为空
        }

        void put(T &&value) {
            MutexLockGuard lock(mutex_);
            // 一直等到到为队列不满，即有空位置
            while (size_ == maxSize_) {
                notFull_.wait();
            }
            assert(size_ < maxSize_);
            queue_.push_back(std::move(value));
            ++size_;
            notEmpty_.notify(); // 通知不为空
        }

        T take() {
            MutexLockGuard lock(mutex_);
            // 一直等到到为队列不为空
            while (size_ == 0) {
                notEmpty_.wait();
            }
            assert(size_ > 0 && size_ <= maxSize_);

            T front(std::move(queue_.front()));
            queue_.pop_front();
            --size_;
            notFull_.notify(); // 通知不满

            return std::move(front);
        }

        size_t size() const {
            MutexLockGuard lock(mutex_);
            return size_;
        }

        size_t capacity() const {
            MutexLockGuard lock(mutex_);
            return maxSize_;
        }

    private:
        mutable MutexLock mutex_;
        Condition notEmpty_;
        Condition notFull_;
        size_t size_;
        size_t maxSize_;
        std::deque<T> queue_; // TODO 循环队列
    };
}

#endif //TINYWS_BOUNDEDBLOCKINGQUEUE_H
