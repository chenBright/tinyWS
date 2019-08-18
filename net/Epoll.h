#ifndef TINYWS_EPOLLER_H
#define TINYWS_EPOLLER_H

#include <sys/epoll.h>
#include <cstdint>

#include <vector>
#include <map>
#include <string>
#include <memory>

#include "../base/noncopyable.h"
#include "EventLoop.h"
#include "Timer.h"

namespace tinyWS {
    class Channel;

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

        explicit Epoll(EventLoop *loop);
        ~Epoll();

        /**
         * --- 只能在 IO 线程中调用 ---
         * 监听文件描述符
         * @param timeoutMs 超时时间
         * @param activeChannels "活动"的 Channel
         * @return 响应时间
         */
        Timer::TimeType poll(int timeoutMs, ChannelList *activeChannels);

        /**
         * --- 只能在 IO 线程中调用 ---
         * 更新 Channel
         * @param channel
         */
        void updateChannel(Channel* channel);

        /**
         * --- 只能在 IO 线程中调用 ---
         * 移除 Channel
         * @param channel
         */
        void removeChannel(Channel* channel);

        /**
         * 断言
         * 判断该调用线程是否为 IO 线程
         */
        void assertInLoopThread();

    private:
        using EventList = std::vector<epoll_event>; // epoll 事件列表类型
        using ChannelMap = std::map<int, Channel*>; // <文件描述符, Channel> 映射

        static const int kInitEventListSize = 16;   // 事件数组（EventList）的默认大小

        EventLoop *ownerLoop_;                      // 所属事件循环
        int epollfd_;                               // epoll 文件描述符
        EventList events_;                          // 活动的文件描述符列表
        ChannelMap channels_;                       // <活动文件描述符, Channel> 映射

        /**
         * 把"活动"的 Channel 填入到 activeChannels 中
         * @param numEvents "活动"的 Channel 数量
         * @param activeChannels "活动"的 Channel 列表
         */
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

        /**
         * 更新 epoll 中的文件描述符
         * @param operation EPOLL_CTL_ADD / EPOLL_CTL_DEL / EPOLL_CTL_MOD
         * @param channel
         */
        void update(int operation, Channel *channel);

        /**
         * 将 epoll 文件描述符操作转换成可读的字符串
         * @param operation 操作
         * @return 操作对应的字符串信息
         */
        std::string operationToString(int operation) const;
    };
}

#endif //TINYWS_EPOLLER_H
