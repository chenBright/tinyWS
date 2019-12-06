#ifndef TINYWS_EPOLL_H
#define TINYWS_EPOLL_H

#include <sys/epoll.h>

#include <vector>
#include <map>
#include <string>

#include "../base/noncopyable.h"
#include "Timer.h"

namespace tinyWS_process2 {
    class Channel;
    class EventLoop;

    // Epoll 的职责：
    // 1 管理（增删改） Channel 列表：有关心事件的 Channel 和 无关心事件的 Channel；
    // 2 Epoll::poll()，监听文件描述符（有关心事件的 Channel 对应的文件描述符）。
    //   当有事件发生时，将"活跃"的 Channel 填充到 activeChannel 中，
    //   供 EventLoop 处理相应的事件，即调用 Channel::handleEvent()。
    //
    // Epoll 是 EventLoop 对象的间接成员，
    // 只供 owner EventLoop 在 IO 线程调用，因此无须加锁。
    // 其生命周期与 EVentLoop 一样长。
    //
    // Epoll 采用的是 level trigger。
    class Epoll : noncopyable {
    public:
        using ChannelList = std::vector<Channel*>;  // Channel 列表类型

    private:
        using EventList = std::vector<epoll_event>; // epoll 事件列表类型
        using ChannelMap = std::map<int, Channel*>; // <文件描述符, Channel> 映射

        static const int kInitEventListSize = 16;   // 事件数组（EventList）的默认大小

        EventLoop* loop_;
        int epollfd_;
        EventList events_;
        ChannelMap channels_;

    public:
        explicit Epoll(EventLoop* loop);

        ~Epoll();

        TimeType poll(int timeoutMS, ChannelList* activeChannels);

        void updateChannel(Channel* channel);

        void removeChannel(Channel* channel);

        bool hasChannel(Channel* channel);

    private:
        void fillActiveChannels(int eventNums, ChannelList *activeChannels) const;

        void update(int operation, Channel* channel);

        std::string operationToString(int operation) const;
    };
}

#endif //TINYWS_EPOLL_H
