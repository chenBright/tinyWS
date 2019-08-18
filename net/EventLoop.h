#ifndef TINYWS_EVENTLOOP_H
#define TINYWS_EVENTLOOP_H

#include <pthread.h>

#include <vector>
#include <memory>
#include <functional>

#include "../base/noncopyable.h"
#include "../base/MutexLock.h"
#include "Timer.h"
#include "TimerId.h"

namespace tinyWS {
    class Channel;
    class Epoll;
    class TimerQueue;

    // IO 线程创建 EventLoop 对象
    // EventLoop 的主要功能是运行事件循环 EventLoop::loop()
    // EventLoop 对象的生命周期和 IO 线程一样长，它不必是 heap 对象
    class EventLoop : noncopyable {
    public:
        using Functor = std::function<void()>; // pending functor 的类型

        EventLoop();
        ~EventLoop();

        // EventLoop::assertInLoopThread() 和 EventLoop::isInLoopThread()
        // 为线程安全（可跨线程调用）的函数和只能在特定线程执行的函数（主要是 IO 线程）
        // 提供检查前提条件的功能

        /**
         * 断言
         * 如果不是在 IO 线程中调用，则退出
         */
        void assertInLoopThread();

        /**
         * 调用线程是否为 IO 线程
         * @return true / false
         */
        bool isInLoopThread() const;

        /**
         * 运行事件循环
         * 必须在 IO 线程中调用该函数
         */
        void loop();

        /**
         * --- 线程安全 ---
         * 退出事件循环
         * 如果通过裸指针调用，非100%线程安全。
         * 如果通过智能指针调用，100%线程安全。
         */
        void quit();

        /**
         * --- 安全线程 ---
         * 在 IO 线程中调用函数
         * 如果在非 IO 线程中调用该函数，会唤醒事件循环，让事件循环处理。
         * 如果在 IO 线程中调用该函数，回调函数直接在该函数中运行。
         *
         * @param cb 回调函数
         */
        void runInLoop(const Functor &cb);

        /**
         * --- 安全线程 ---
         * 将回调函数放到队列中，并在必要时唤醒 IO 线程。
         * 队列中的回调函数在调用完时间回调函数后被调用。
         * @param cb 回调函数
         */
        void queueInLoop(const Functor &cb);

        /**
         * --- 安全线程 ---
         * 在指定时间调用 cb
         * @param time 调用回调函数的时刻
         * @param cb 回调函数
         * @return 定时器 id
         */
        TimerId runAt(Timer::TimeType time, const Timer::TimeCallback &cb);

        /**
         * --- 安全线程 ---
         * 延迟一段时间调用回调函数
         * @param delay 延迟的时间
         * @param cb 回调函数
         * @return 定时器 id
         */
        TimerId runAfter(Timer::TimeType delay, const Timer::TimeCallback &cb);

        /**
         * --- 安全线程 ---
         * 周期调用回调函数
         * @param interval 周期
         * @param cb 回调函数
         * @return 定时器 id
         */
        TimerId runEvery(Timer::TimeType interval, const Timer::TimeCallback &cb);

        /**
         * --- 安全线程 ---
         * 注销定时任务
         * @param timerId 定时器 id
         */
        void cancle(TimerId timerId);

        /**
         * 内部使用
         * 唤醒 IO 线程，处理
         */
        void wakeup();


         // EVentLoop 不关心 Epoll 如何管理 Channel 列表，
         // 所以在 updateChannel 内部直接调用 Epoll::updateChannel
         // 在 removeChannel 内部直接调用 Epoll::removeChannel

        /**
         * 更新 channel
         *
         * @param channel
         */
        void updateChannel(Channel *channel);

        /**
         * 移除 channel
         *
         * @param channel
         */
        void removeChannel(Channel *channel);

        /**
         * 获取线程的事件循环
         *
         * @return EventLoop 对象
         */
        static EventLoop* getEventLoopOfCurrentThread();

    private:
        using ChannelList = std::vector<Channel*>;  // Channel 列表类型

        bool looping_;                              // 是否在事件循环中
        bool quit_;                                 // 是否退出事件循环
        bool callingPendingFuntors_;                // 是否正在处理 pending functor
        const pid_t threadId_;                      // EventLoop 所属线程ID
        std::unique_ptr<Epoll> epoll_;              // Epoll 对象指针
        std::unique_ptr<TimerQueue> timerQueue_;    // 定时器队列
        int wakeupFd_;                              // 用于唤醒 IO 线程的文件描述符
        std::unique_ptr<Channel> wakeupChannel_;    // 不需要像内部类 TimerQueue 一样暴露给客户端，不需共享所有权
        ChannelList activeChannels_;                // "活跃"的 Channel，表示有时间需要处理
        MutexLock mutex_;                           // 互斥量对象
        std::vector<Functor> pendingFunctors_;      // 需要在 IO 线程执行的"任务"（函数）

        /**
         * 如果不在 IO 线程中调用此函数，则打印 IO 线程信息
         */
        void abortNotInLoopThread();

        /**
         * IO线程被唤醒后，读取 wakeupfd 文件描述符中的数据。
         * 因为程序的模式为 LT（水平触发），如果不将数据读出来，epoll 会不停触发通知。
         */
        void handleRead();  // waked up

        /**
         * 调用 pending functor
         */
        void doPendingFunctors();

        /**
         * TODO 打印"活跃"的 Channel 的信息
         */
        void printActiveChannels() const; // for DEBUG
    };
}


#endif //TINYWS_EVENTLOOP_H
