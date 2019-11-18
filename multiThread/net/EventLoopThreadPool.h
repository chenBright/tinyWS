#ifndef TINYWS_EVENTLOOPTHREADPOOL_H
#define TINYWS_EVENTLOOPTHREADPOOL_H

#include <memory>
#include <vector>
#include <functional>

#include "../base/noncopyable.h"

namespace tinyWS_thread {
    class EventLoop;
    class EventLoopThread;

    // 用 one loop per thead 思想实现的多线程 TcpServer 的关键步骤：
    // 在创建 TcpConnection 是从 event loop pool 里选一个 EventLoop 来使用。
    // 即多线程 TcpServer 所属的 EventLoop 只用来接受新连接，
    // 而新连接会使用 event loop pool 来执行 IO 操作。
    // 而单线程 TcpServer 的所有工作都在 TcpServer 所属的 EventLoop 做。
    //
    // 当前选择 EventLoop 的策略是 Round-robin。
    class EventLoopThreadPool : noncopyable {
    public:
        using EventLoopThreadPoolCallback = std::function<void(EventLoop*)>; // 线程池回调函数类型

        /**
         * 构造函数
         * @param baseLoop 主 EventLoop
         */
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
        EventLoop *baseLoop_;                                       // 主 EventLoop
        bool started_;                                              // 线程池是否启动
        int numThreads_;                                            // 线程数
        int next_;                                                  // 用于获取下一线程
        std::vector<std::unique_ptr<EventLoopThread> > threads_;    // 线程列表
        std::vector<EventLoop*> loops_;                             // EventLoop 列表
    };
}

#endif //TINYWS_EVENTLOOPTHREADPOOL_H
