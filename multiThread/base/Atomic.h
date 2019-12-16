#ifndef TINYWS_ATOMIC_H
#define TINYWS_ATOMIC_H

#include <cstdint>

#include "noncopyable.h"

namespace tinyWS_thread {
    template <class T>
    class AtomicIntegerT : noncopyable {
    public:
        AtomicIntegerT() : value_(0) {}

        T get() {
            return __atomic_load_n(&value_, __ATOMIC_SEQ_CST);
        }

        T getAndAdd(T x) {
            return __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST);
        }

        T addAndGet(T x) {
            return getAndAdd(x) + x;
        }

        T incrementAndGet() {
            return addAndGet(1);
        }

        T decrementAndGet() {
            return addAndGet(-1);
        }

        void add(T x) {
            getAndAdd(x);
        }

        void increment() {
            incrementAndGet();
        }

        void decrement() {
            decrementAndGet();
        }

        T getAndSet(T newValue) {
            return __atomic_exchange_n(&value_, newValue, __ATOMIC_SEQ_CST);
        }
    private:
        // volatile 关键字，参考：
        // https://zh.cppreference.com/w/cpp/language/cv
        // https://liam.page/2018/01/18/volatile-in-C-and-Cpp/
        volatile T value_;
    };

    using AtomicInt32 = AtomicIntegerT<int32_t>;
    using AtomicInt64 = AtomicIntegerT<int64_t>;
}


#endif //TINYWS_ATOMIC_H
