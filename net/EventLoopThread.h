#ifndef TINYWS_EVENTLOOPTHREAD_H
#define TINYWS_EVENTLOOPTHREAD_H

#include <functional>

#include "../base/noncopyable.h"
#include "../base/MutexLock.h"
#include "../base/Condition.h"
#include "../base/Thread.h"

namespace tinyWS {
    class EventLoop;

    // EventLoopThread 启动自己的线程，
    // 在其中运行 EventLoop::loop，并返回 EventLoop 指针
    class EventLoopThread : noncopyable {
    public:
        typedef std::function<void(EventLoop*)> EventLoopThreadCallback;    // 线程回调函数类型

        /**
         * 构造函数
         * @param cb 回调函数
         */
        explicit EventLoopThread(const EventLoopThreadCallback &cb = EventLoopThreadCallback());

        ~EventLoopThread();

        /**
         * 启动线程，并在创建的线程中执行 EventLoop::loop()
         * @return EventLoop 所属 EventLoop
         */
        EventLoop* startThread();

    private:
        // EventLoop 的生命周期与线程主函数的作用域相同，
        // 因此在 threadFunction 退出后，这个指针就失效了。
        // 但服务器程序一般不要求能安全地退出，这应该不是什么大问题。
        EventLoop *loop_;                                                   // 所属 EventLoop
        bool exiting_;                                                      // 是否已经创建 EventLoop
        Thread thread_;                                                     // 线程
        MutexLock mutex_;                                                   // 互斥量
        Condition cond_;                                                    // 条件变量
        EventLoopThreadCallback callback_;                                  // 回调函数

        /**
         * 启动线程执行的函数
         */
        void threadFunction();
    };
}


#endif //TINYWS_EVENTLOOPTHREAD_H
