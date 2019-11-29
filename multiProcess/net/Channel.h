#ifndef TINYWS_CHANNEL_H
#define TINYWS_CHANNEL_H

#include <functional>
#include <memory>
#include <string>

#include "../base/noncopyable.h"
#include "type.h"
#include "TimerQueue.h"

namespace tinyWS_process {
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

    public:
        explicit Channel(EventLoop* loop, int fdArg);

        ~Channel() = default;

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

        EventLoop* ownerLoop();

        void remove();

        // for debug
        std::string reventsToString() const;

        std::string eventsToString() const;

    private:
        void update();

        std::string eventsToString(int fd, int event) const;
    };
}


#endif //TINYWS_CHANNEL_H
