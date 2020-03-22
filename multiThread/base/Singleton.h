#ifndef TINYWS_SINGLETON_H
#define TINYWS_SINGLETON_H

#include <pthread.h>
#include <cstdint>
#include <cstdlib>
#include <cassert>

#include "noncopyable.h"

namespace tinyWS_thread {

    // 参考
    // https://segmentfault.com/q/1010000000593968
    // https://www.cnblogs.com/loveis715/archive/2012/07/18/2598409.html
    // https://blog.csdn.net/lsaejn/article/details/78409001
    // http://kaiyuan.me/2018/05/08/sfinae/
    // https://www.jianshu.com/p/45a2410d4085
    // https://zhuanlan.zhihu.com/p/21314708
    // http://kaiyuan.me/2018/05/08/sfinae/

    /**
     * 判断 T 类型是否有 no_destroy 方法。
     * 如果有，则 value == true；
     * 否则，value == false。
     *
     * @tparam T
     */
    template <class T>
    struct has_no_destroy {
        // 使用了 SFINAE 技术，即匹配失败不是错误，在编译时期来确定某个 type 是否具有我们需要的性质。
        // 此时用于确定 T 类型是否有 no_destroy 方法。
        // 如果 T 是 POD 类型，没有 no_destroy 方法，不可以使用 &C::no_destroy 方式调用，
        // 意味这如果匹配 test(decltype(&C::no_destroy)) 函数会导致编译错误，
        // 那么它会寻找下一个函数去实例化。
        // 因为 test(...) 的存在，任何不匹配上一个函数的到这里都会匹配，
        // 所以声明的成员函数 test 是的返回类型是 int32_t，而不是 char。
        // sizeof(int32_t) != 1，所以 value 的值为 false。
        // 参考 http://kaiyuan.me/2018/05/08/sfinae/
        template <class C> static char test(decltype(&C::no_destroy));
        template <class C> static int32_t test(...);

        const static bool value = sizeof(test<T>(0)) == 1;
    };

    template <typename T>
    class Singleton : noncopyable {
    private:
        // 使用 pthread_once_t 来保证 lazy-initialization 的线程安全，线程安全性由 Pthread 库保证。
        static pthread_once_t ponce_;
        static T *value_;

    public:
        static T& instance() {
            // 使用 pthread_once 函数，保证对象只会初始化一次。
            pthread_once(&ponce_, &Singleton::init);
            assert(value_ != nullptr);

            return *value_;
        }

    private:
        Singleton() = default;
        ~Singleton() = default;

        static void init() {
            value_ = new T();
            // 如果类型 T 没有 no_destroy 方法，即有 destroy 的方式，
            // 则注册终止函数，在程序退出前清理资源。
            if (!has_no_destroy<T>::value) {
                ::atexit(destory);
            }
        }

        static void destory() {
            // 在编译期间检查不完全类型错误。
            // 用 typedef 定义了一个数组类型。
            // 因为数组的大小不能为 -1，如果 T 是不完全类型，
            // sizeof(T) == 0，则数组大小为 -1，编译阶段就会发现错误。
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void)(dummy);

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
