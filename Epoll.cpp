#include "Epoll.h"

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h> // close
#include <strings.h> // bzero

#include <iostream>

#include "Channel.h"

using namespace tinyWS;

// channel 的状态
const int kNew = -1; // 表示还没添加到 ChannelMap 中
const int kAdded = 1; // 已添加到 ChannelMap 中
const int kDeleted = 2; // 无关心事件，已从 epoll 中删除相应文件描述符，但 ChannelMap 中有记录

Epoll::Epoll(EventLoop *loop)
    : ownerLoop_(loop),
      epollfd_(epoll_create1(EPOLL_CLOEXEC)), // TODO 学习 epoll_create1 和 EPOLL_CLOEXEC
      events_(kInitEventListSize) {

    if (epollfd_ < 0) {
        std::cout << "EPollPoller::EPollPoller" << std::endl;
    }
}

Epoll::~Epoll() {
    close(epollfd_);
}

Timer::TimeType Epoll::poll(int timeoutMs, Epoll::ChannelList *activeChannels)  {
    // 往 events_ 的内存里写入活跃的事件
    int eventNums = epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    // TODO 此处暴露了 Timer
    Timer::TimeType now = Timer::now();

    if (eventNums > 0) {
        std::cout << "events happen" << std::endl;
        fillActiveChannels(eventNums, activeChannels);
        // 当向epoll中注册的事件过多，导致返回的活动事件可能越來越多，events_ 裝不下时，events_ 扩容为2倍
        if (static_cast<EventList::size_type>(eventNums) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (eventNums == 0) {
        std::cout << "nothing happended" << std::endl;
    } else {
        std::cout << "EPollPoller::poll()" << std::endl;
    }

    return now; // 返回 epoll return 的时刻
}

void Epoll::updateChannel(Channel *channel) {
    assertInLoopThread();
    std::cout << "fd = " << channel->fd() << " event = " << channel->events() << std::endl;
    const int index = channel->statusInEpoll();
    int fd = channel->fd();
    if (index == kNew || index == kDeleted) {
        // a new one, add with EPOLL_CTL_ADD
        if (index == kNew) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->setStatusInEpoll(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setStatusInEpoll(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoll::removeChannel(Channel *channel) {
    assertInLoopThread();
    int fd = channel->fd();
    channels_.erase(fd);
    if (channel->statusInEpoll() == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setStatusInEpoll(kNew);
}

void Epoll::assertInLoopThread() {
    ownerLoop_->assertInLoopThread();
}

void Epoll::fillActiveChannels(int numEvents, Epoll::ChannelList *activeChannels) const {
    assert((size_t) numEvents <= events_.size());
    for (int i = 0; i < numEvents; ++i) {
        auto *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void Epoll::update(int operation, Channel *channel) {
    epoll_event event{};
    bzero(&event, sizeof(event));

    /**
     * struct epoll_event {
     *     unit32_t events;
     *     epoll_data_t data;
     * }
     * struct union epoll_data {
     *     void *ptr;
     *     int fd;
     *     uint32_t u32;
     *     uint64_t u64;
     * } epoll_data_t;
     *
     * 使用 ptr 成员保存 channel 指针
     */
    event.events = static_cast<uint32_t>(channel->events());
    event.data.ptr = channel;
    int fd = channel->fd();
    if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        std::cout << "epoll_ctl op=" << operation << " fd=" << fd;
    }
}