#ifndef TINYWS_SINGLETON_H
#define TINYWS_SINGLETON_H

#include <pthread.h>

#include "noncopyable.h"

namespace tinyWS {
    /**
     * 使用 pthread_once_t 来保证 lazy-initialization 的线程安全，线程安全性由 Pthread 库保证。
     * @tparam T
     */
    template <typename T>
    class Singleton : noncopyable {
    public:
        static T& instance() {
            pthread_once(&ponce_, &Singleton::init);
            return *value_;
        }

    private:
        static pthread_once_t ponce_;
        static T *value_;

        Singleton();
        ~Singleton();

        static void init() {
            value_ = new T();
        }

        static void destory() {
            // TODO 不懂
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            static_cast<void>(dummy);

            delete value_;
            value_ = nullptr;
        }
    };

// 必须在头文件中定义 static 变量
    template <typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template <typename T>
    T* Singleton<T>::value_ = nullptr;
}

#endif //TINYWS_SINGLETON_H
