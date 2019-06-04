#ifndef TINYWS_THREADPOOL_H
#define TINYWS_THREADPOOL_H

#include <functional>
#include <memory>
#include <vector>
#include <deque>
#include <string>

#include "noncopyable.h"
#include "Thread.h"
#include "MutexLock.h"
#include "Condition.h"

namespace tinyWS {
    class ThreadPool : noncopyable {
    public:
        // 任务函数
        typedef std::function<void()> Task;

        /**
         * 构造函数
         * @param name 线程池名
         */
        explicit ThreadPool(const std::string &name = std::string());
        ~ThreadPool();

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
        void run(const Task &task);

    private:
        typedef  std::vector<std::unique_ptr<Thread> > ThreadList; // 线程列表类型
        MutexLock mutex_; // 互斥锁
        Condition cond_; // 条件变量
        std::string name_; // 线程池名
        ThreadList threads_; // 线程列表
        std::deque<Task> dequeue_; // 任务队列（双向队列）
        bool running_; // 线程池是否启动

        /**
         * 执行任务
         */
        void runInThread();

        /**
         * 获取任务
         * @return 任务函数
         */
        Task take();
    };
}


#endif //TINYWS_THREADPOOL_H
