#ifndef TINYWS_CHANNEL_H
#define TINYWS_CHANNEL_H

#include <functional>
#include <memory>
#include <string>

#include "../base/noncopyable.h"
#include "type.h"

namespace tinyWS_process2 {
    class EventLoop;

    class Channel : noncopyable {
    public:
        using EventCallback = std::function<void()>;                    // 事件回调函数
        using ReadEventCallback = std::function<void(TimeType)>; // 读事件回调函数

    private:
        static const int kNoneEvent;        // 无事件
        static const int kReadEvent;        // 读事件
        static const int kWriteEvent;       // 写事件

        EventLoop* loop_;
        int fd_;
        int events_;
        int revents_;

        int statusInEpoll_;

        ReadEventCallback readCallback_;    // 读事件回调函数
        EventCallback writeCallback_;       // 写事件回调函数
        EventCallback closeCallback_;       // 关闭事件回调函数
        EventCallback errorCallback_;       // 异常事件回调函数

        // 详情见《Linux多线程服务端编程》P274
        // 如果用户在 onClose() 中析构 Channel 对象，这会造成要种的后果。
        // 即 Channel::handleEvent() 执行到一半，其所属的 Channel 独享本身被销毁了。
        // 这时程序立即 core dump 就是最好的结果了。
        // 解决方法：提供接口 Channel::tie()，用于延长某些对象（可以是 Channel，也可以是其 owner 对象）的生命期,
        // 使之长过 Channel::handleEvent() 函数。
        // 这也是 TcpConnection 使用 shared_ptr 管理对象生命期的原因之一。
        bool eventHandling_;                // 是否在处理事件，用于调试阶段保证 Channel 不会在处理事件过程中析构
        bool addedToLoop_;                  // 是否添加到 EventLoop 中
        std::weak_ptr<void> tie_;           // 绑定对象
        bool tied_;                         // 是否绑定了对象

    public:
        explicit Channel(EventLoop* loop, int fdArg);

        ~Channel();

        void tie(const std::shared_ptr<void>& obj);

        void handleEvent(TimeType receiveTime);

        void setReadCallback(const ReadEventCallback& cb);

        void setWriteCallack(const EventCallback& cb);

        void setCloseCallback(const EventCallback& cb);

        void setErrorCallback(const EventCallback& cb);

        int fd() const;

        int getEvents() const;

        int setRevents(int revt);

        bool isNoneEvent() const;

        void enableReading();

        void enableWriting();

        void disableWriting();

        void disableAll();

        bool isWriting() const;

        int getStatusInEpoll();

        void setStatusInEpoll(int statusInEpoll);

        void resetLoop(EventLoop* loop);

        EventLoop* ownerLoop();

        void remove();

        // for debug
        std::string reventsToString() const;

        std::string eventsToString() const;

    private:
        void handleEventWithGuard(TimeType receiveTime);

        void update();

        std::string eventsToString(int fd, int event) const;
    };
}


#endif //TINYWS_CHANNEL_H
