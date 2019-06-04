#ifndef TINYWS_EVENTLOOPTHREADPOOL_H
#define TINYWS_EVENTLOOPTHREADPOOL_H

#include <memory>
#include <vector>
#include <functional>

#include "base/noncopyable.h"

namespace tinyWS {
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool : noncopyable {
    public:
        typedef std::function<void(EventLoop*)> EventLoopThreadPoolCallback;

        explicit EventLoopThreadPool(EventLoop *baseLoop);
        ~EventLoopThreadPool();

        /**
         * 设置线程池线程数目
         * @param numThreads
         */
        void setThreadNum(int numThreads);

        /**
         * 启动事件循环线程池，创建线程
         * @param cb 线程回调函数
         */
        void start(const EventLoopThreadPoolCallback &cb = EventLoopThreadPoolCallback());

        /**
         * 获取 EventLoop
         * @return  EventLoop
         */
        EventLoop *getNextLoop();

    private:
        EventLoop *baseLoop_; // 主事件循环
        bool started_; // 线程池是否启动
        int numThreads_; // 线程数
        int next_; // 用于获取下一线程
        std::vector<std::unique_ptr<EventLoopThread> > threads_; // 线程列表
        std::vector<EventLoop*> loops_; // 事件循环列表
    };
}

#endif //TINYWS_EVENTLOOPTHREADPOOL_H
