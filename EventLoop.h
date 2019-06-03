#ifndef TINYWS_EVENTLOOP_H
#define TINYWS_EVENTLOOP_H

#include <pthread.h>

#include <vector>
#include <memory>
#include <functional>

#include "base/noncopyable.h"
#include "Timer.h"
#include "base/MutexLock.h"
#include "TimerId.h"

namespace tinyWS {
    class Channel;
    class Epoll;
    class TimerQueue;

    // EventLoop 对象的生命周期和所属的线程一样长
    class EventLoop : noncopyable {
    public:
        typedef std::function<void()> Functor;

        EventLoop();
        ~EventLoop();

        /**
         * 断言
         * 如果不是在创建线程中调用，则退出
         */
        void assertInLoopThread();

        /**
         * 调用线程是否为创建线程
         * @return true / false
         */
        bool isInLoopThread() const;

        /**
         * 运行时间循环
         * 必须在创建对象的线程中调用
         */
        void loop();

        /**
         * 退出事件循环
         * 如果通过裸指针调用，非100%线程安全。
         * 如果通过智能指针调用，100%线程安全。
         */
        void quit();

        /**
         * --- 安全线程 ---
         * 在事件循环中调用回调函数，会唤醒事件循环，运行事件循环。
         * 如果在同一事件循环线程中，回调函数直接在该函数中运行。
         *
         * @param cb 回调函数
         */
        void runInLoop(const Functor &cb);

        /**
         * --- 安全线程 ---
         * 将回调函数放到队列中，并在必要时唤醒 IO 线程。
         * 队列中的回调函数在事件循环完成后运行。
         *
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

        // 内部使用
        void wakeup();

        /**
         * EVentLoop 不关心 Epoll 如何管理 Channel 列表，
         * 所以在 updateChannel 内部直接调用 Epoll::updateChannel
         * 在 removeChannel 内部直接调用 Epoll::removeChannel
         */

        /**
         * 更新 channel
         * @param channel
         */
        void updateChannel(Channel *channel);

        /**
         * 移除 channel
         * @param channel
         */
        void removeChannel(Channel *channel);

        /**
         * 获取线程的事件循环
         * @return EventLoop 对象
         */
        static EventLoop* getEventLoopOfCurrentThread();

    private:
        typedef std::vector<Channel*> ChannelList;

        bool looping_; // 是否在事件循环中
        bool quit_; // 是否退出事件循环
        bool callingPendingFuntors_; // 是否正在处理 pendingFunctor
        const pid_t threadId_; // EventLoop 所属线程ID
        std::unique_ptr<Epoll> epoll_; // Epoll 对象指针
        std::unique_ptr<TimerQueue> timerQueue_; // 定时器队列
        int wakeupFd_; // 用于唤醒 IO 线程的文件描述符
        std::unique_ptr<Channel> wakeupChannel_; // 不需要像内部类 TimerQueue 一样暴露给客户端，不需共享所有权
        ChannelList activeChannels_;
        MutexLock mutex_;
        std::vector<Functor> pendingFunctors_;

        /**
         * 如果不在 IO 线程中，打印信息
         */
        void abortNotInLoopThread();

        /**
         * IO线程被唤醒后，读取 wakeupfd 文件描述符中的数据
         * 因为程序的模式为 LT（水平触发），如果不将数据读出来，epoll 会不停触发通知
         */
        void handleRead();  // waked up
        void doPendingFunctors();

        void printActiveChannels() const; // DEBUG
    };
}


#endif //TINYWS_EVENTLOOP_H
