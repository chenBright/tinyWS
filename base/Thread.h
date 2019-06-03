#ifndef TINYWS_THREAD_H
#define TINYWS_THREAD_H

#include <pthread.h>

#include <string>
#include <functional>

#include "noncopyable.h"

namespace tinyWS {
    class Thread : noncopyable {
    public:
        // 线程执行函数
        typedef std::function<void()> ThreadFunction;

        /**
         * 构造函数
         * @param func 线程执行函数
         * @param name 线程名
         */
        explicit Thread(const ThreadFunction &func, const std::string &name = std::string());
        ~Thread();

        /**
         * 创建线程
         */
        void start();

        /**
         * 线程是否创建
         * @return
         */
        bool started();

        /**
         * join
         * @return  pthread_join()
         */
        int join();

        /**
         * 获取线程 tid
         * @return 系统调用得到的 tid
         */
        pid_t tid() const;

        /**
         * 获取线程名
         * @return 线程名
         */
        const std::string& name() const;

        /**
         * 获取线程 id
         * TODO
         * 函数内部使用了系统调用，每次都陷入内核。优化：可使用 __thread 修饰的变量缓存线程 id
         * @return
         */
        static pid_t gettid();

    private:
        bool started_; // 线程是否启动
        pthread_t pthreadId_; // pthread库返回的线程id
        pid_t tid_; // 系统调用返回的线程id
        ThreadFunction func_; // 线程执行函数
        std::string name_; // 线程名

        /**
         * 启动线程
         * @param obj 线程对象
         * @return nullptr
         */
        static void* startThread(void *obj);

        /**
         * 在线程中指向注册的函数
         */
        void runInThread();
    };
}

#endif //TINYWS_THREAD_H
