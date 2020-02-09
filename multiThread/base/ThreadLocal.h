#ifndef TINYWS_THREADLOCAL_H
#define TINYWS_THREADLOCAL_H

#include <pthread.h>
#include <thread>
#include "noncopyable.h"

namespace tinyWS_thread {

    template <class T>
    class ThreadLocal : noncopyable {
    private:
        pthread_key_t pkey_;

    public:
        ThreadLocal() {
            ::pthread_key_create(&pkey_, &ThreadLocal::destructor);
        }

        ~ThreadLocal() {
            pthread_key_delete(pkey_);
        }

        T& value() {
            T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
            if (!perThreadValue) {
                T* newObj = new T();
                pthread_setspecific(pkey_, newObj);
                perThreadValue = newObj;
            }

            return *perThreadValue;
        }

    private:
        static void destructor(void* x) {
        T* obj = static_cast<T*>(x);
        delete obj;
    }
    };
}

#endif //TINYWS_THREADLOCAL_H
