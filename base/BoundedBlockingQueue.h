#ifndef TINYWS_BOUNDEDBLOCKINGQUEUE_H
#define TINYWS_BOUNDEDBLOCKINGQUEUE_H

#include <cassert>

#include <deque>
#include <vector>
#include <exception>

#include "noncopyable.h"
#include "MutexLock.h"
#include "Condition.h"

namespace tinyWS {

    // 循环队列，元素范围为 [front_, rear_)，
    // 即逻辑上的队列容量为 capacity，而实际的空间大小为 capcaity + 1，
    // 其中 rear_ 指向的位置为空或者已经不是循环队列中的元素（被覆盖了）。
    template <class T>
    class circular_buffer {
    public:
        explicit circular_buffer(int capacity)
            : size_(0),
              capacity_(capacity + 1), // 预留一个空位置
              data_(new T[capacity]),
              front_(0),
              rear_(0) {
            assert(capacity > 0);
        }

        /**
         * 获取元素的个数。
         * @return 元素个数
         */
        size_t size() const {
            return size_;
        }

        /**
         * h获取队列的容量。
         * @return 队列容量
         */
        size_t capacity() const {
            return capacity_;
        }

        /**
         * 队列是否满了。
         * @return true / false
         */
        bool full() const {
            return size_ == capacity_;
        }

        /**
         * 队列是否为空
         * @return
         */
        bool empty() {
            return size_ == 0;
        }

        /**
         * 获取队头元素。
         * @return 队头元素
         */
        T front() const {
            // 如果队列为空，则抛出异常。
            if (empty()) {
                throw std::bad_exception();
            }

            return data_[front_];
        }

        /**
         * 获取队尾元素。
         * @return 队尾元素
         */
        T back() const {
            // 如果队列为空，则抛出异常。
            if (empty()) {
                throw std::bad_exception();
            }

            int backIndex = rear_ == 0 ? capacity_ : rear_ - 1;
            return data_[backIndex];
        }

        /**
         * 队头元素出队。
         */
        void pop_front() {
            // 如果队列为空，则抛出异常。
            if (empty()) {
                throw std::bad_exception();
            }

            front_ = (front_ + 1) % (capacity_ + 1);
        }

        void push_back(T value) {
            data_[rear_] = value;
            rear_ = (rear_ + 1) % (capacity_ + 1);
            if (full()) {
                front_ = (front_ + 1) % (capacity_ + 1);
            }
        }

    private:
        size_t size_;       // 元素个数
        size_t capacity_;   // 队列容量
        T *data_;           // 数据数组
        int front_;         // 队头指针
        int rear_;          // 队尾指针，指向的位置是空的。
    };

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
        std::deque<T> queue_;
    };
}

#endif //TINYWS_BOUNDEDBLOCKINGQUEUE_H
