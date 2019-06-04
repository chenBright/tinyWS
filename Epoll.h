#ifndef TINYWS_EPOLLER_H
#define TINYWS_EPOLLER_H

#include <sys/epoll.h>
#include <cstdint>

#include <vector>
#include <map>
#include <memory>

#include "base/noncopyable.h"
#include "EventLoop.h"
#include "Timer.h"

namespace tinyWS {
    class Channel;

    /**
     * Epoll 对象是 EventLoop 对象的间接成员，
     * 只供 owner EventLoop 对象在 IO线程调用，因此无须加锁。
     * 其生命周期与 EVentLoop 一样长。
     */
    class Epoll : noncopyable {
    public:
        typedef std::vector<Channel*> ChannelList; // Channel 列表类型

        explicit Epoll(EventLoop *loop);
        ~Epoll();

        Timer::TimeType poll(int timeoutMs, ChannelList *activeChannels);
        void updateChannel(Channel* channel);
        void removeChannel(Channel* channel);

        void assertInLoopThread();

    private:
        typedef std::vector<epoll_event> EventList; // epoll 事件列表类型
        typedef std::map<int, Channel*> ChannelMap; // 文件描述符 - Channel

        static const int kInitEventListSize = 16; // 默认事件数组大小

        EventLoop *ownerLoop_;
        int epollfd_; // epoll 文件描述符
        EventList events_; // 活动的文件描述符列表
        ChannelMap channels_; // 活动文件描述符 - Channel

        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
        void update(int operation, Channel *channel);
    };
}

#endif //TINYWS_EPOLLER_H
