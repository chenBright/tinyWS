#ifndef TINYWS_THREADPOOL_CPP11_H
#define TINYWS_THREADPOOL_CPP11_H

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <string>

#include "noncopyable.h"

namespace tinyWS_thread {

    // 线程池 C++ 11 实现
    // 参考
    // https://wangpengcheng.github.io/2019/05/17/cplusplus_theadpool/
    // http://www.cclk.cc/2017/11/14/c++/c++_threadpool/
    class ThreadPool_cpp11 : noncopyable {
    public:
        using Task = std::function<void()>;                             // 任务函数类型

    private:
        using ThreadList = std::vector<std::thread>;       // 线程列表类型
        std::mutex mutex_;
        std::condition_variable cond_;
        std::string name_;                                              // 线程池名
        ThreadList threads_;                                            // 线程列表
        std::queue<Task> queue_;                                      // 任务队列（双向队列）
        bool running_;                                                  // 线程池是否启动

    public:
        /**
         * 构造函数
         * @param name 线程池名
         */
        explicit ThreadPool_cpp11(const std::string& name = std::string());

        ~ThreadPool_cpp11();

        /**
         * 初始化线程池
         * @param numThreads 线程池线程数
         */
        void start(int numThreads);

        /**
         * 关闭线程池，并等待线程终止
         */
        void stop();

        /**
         * 执行任务
         * @param task 任务函数
         */
        void run(const Task& task);

    private:
        /**
        * 不断地从任务队列中取出任务执行
        */
        void runInThread();

        /**
         * 获取任务
         * @return 任务函数
         */
        Task take();
    };
}


#endif //TINYWS_THREADPOOL_CPP11_H
