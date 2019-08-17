#ifndef TINYWS_CHANNEL_H
#define TINYWS_CHANNEL_H

#include <functional>

#include "../base/noncopyable.h"
#include "Timer.h"

namespace tinyWS {
    class EventLoop;

    // A selectable I/O channel
    //
    // 每个 Channel 对象都只属于某一个 IO 线程，
    // 自始至终值负责一个文件描述符（fd），
    // 但它并不拥有这个 fd，也不会在析构的时候关闭它。
    class Channel : noncopyable {
    public:
        typedef std::function<void()> EventCallback;                    // 事件回调函数
        typedef std::function<void(Timer::TimeType)> ReadEventCallback; // 读事件回调函数

        /**
         * 构造函数
         * @param loop 所属事件循环
         * @param fd 所负责的文件描述符
         */
        Channel(EventLoop *loop, int fd);

        ~Channel();

        /**
         * 处理事件
         */
        void handleEvent(Timer::TimeType receiveTime);

        /**
         * 设置读事件回调函数
         * @param cb 回调函数
         */
        void setReadCallback(const ReadEventCallback &cb);

        /**
         * 设置写事件回调函数
         * @param cb 回调函数
         */
        void setWriteCallback(const EventCallback &cb);

        /**
         * 设置关闭事件回调函数
         * @param cb 回调函数
         */
        void setCloseCallback(const EventCallback &cb);

        /**
         * 设置异常事件回调函数
         * @param cb 回调函数
         */
        void setErrorCallback(const EventCallback &cb);

        /**
         * 获取文件描述符
         * @return 文件描述符
         */
        int fd() const;

        /**
         * 获取 IO 事件
         * @return 事件
         */
        int getEvents() const;

        /**
         * 设置活动事件
         * @param revt 事件
         */
        void setRevents(int revt);

        /**
         * 是否无事件
         * @return true / false
         */
        bool isNoneEvent() const;

        /**
         * 设置可读
         */
        void enableReading();

        /**
         * 设置可写
         */
        void enableWriting();

        /**
         * 设置不可写
         * 专门为写事件设置一个函数来设置不可写的原因：
         * Epoll 采用的是 level trigger，只需要在需要时关注写事件。
         * 否则 socket fd 一直可写，会唤醒 IO 线程，造成 busy loop。
         */
        void disableWriting();

        /**
         * 设置全部事件不可用
         */
        void disableAll();

        /**
         * 是否正在写数据
         * @return true / false
         */
        bool isWriting() const;

        /**
         * 返回 statusInEpoll_，即 Channel 在 Epoll 中的状态
         * @return statusInEpoll_ Channel 在 Epoll 中的状态
         */
        int getStatusInEpoll();

        /**
         * 设置 statusInEpoll_，即 Channel 在 Epoll 中的状态
         * @param statusInEpoll Channel 在 Epoll 中的状态
         */
        void setStatusInEpoll(int statusInEpoll);

        /**
         * 获取 EventLoop 的指针
         * @return loop_ EventLoop 的指针
         */
        EventLoop* ownerLoop();

        /**
         * 从 EventLoop 中移除该 Channel
         */
        void remove();

    private:
        static const int kNoneEvent;        // 无事件
        static const int kReadEvent;        // 读事件
        static const int kWriteEvent;       // 写事件

        EventLoop *loop_;                   // 所属事件循环
        const int fd_;                      // 负责的文件描述符
        int events_;                        // IO事件，由用户设置。bit pattern
        int revents_;                       // 目前活动事件，由 EventLoop / Epoll 设置。bit pattern

        // 给 Epoll 使用，表示 Channel 的状态（全新、已添加、已删除），
        // 因为 Channel 并不会使用到该状态值，并不关心改状态值的类型和值，
        // 只有 Epoll 对象会通过调用 Channel::getStatusInEpoll() 和 Channel::setStatusInEpoll() 来操作该状态值，
        // 所以状态定义在 Epoll 类中。
        // const int kNew = -1; // 表示还没添加到 ChannelMap 中
        // const int kAdded = 1; // 已添加到 ChannelMap 中
        // const int kDeleted = 2; // 无关心事件，已从 epoll 中删除相应文件描述符，但 ChannelMap 中有记录
        int statusInEpoll_;

        ReadEventCallback readCallback_;    // 读事件回调函数
        EventCallback writeCallback_;       // 写事件回调函数
        EventCallback closeCallback_;       // 关闭事件回调函数
        EventCallback errorCallback_;       // 异常事件回调函数

        bool eventHandling_;                // 是否在处理事件，用于调试阶段保证 Channel 不会在处理事件过程中析构

        /**
         * 更新 Channel 在对应 Epoll 中的信息，包括文件描述符事件、Channel 状态。
         * Channel::update 会调用 EventLoop:updateChannel，
         * 后者会转而调用 Epoll::updateChannel。
         */
        void update();
    };
}


#endif //TINYWS_CHANNEL_H
