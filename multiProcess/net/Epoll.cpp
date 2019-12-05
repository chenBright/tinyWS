#include "Epoll.h"

#include <unistd.h>
#include <cassert>

#include <iostream>

#include "EventLoop.h"
#include "Channel.h"

using namespace tinyWS_process;

// channel 的状态
// 表示还没添加到 ChannelMap 中
const int kNew = -1;
// 已添加到 ChannelMap 中
const int kAdded = 1;
// 无关心事件，已从 epoll 中删除相应文件描述符，但 ChannelMap 中有记录
const int kDeleted = 2;

Epoll::Epoll(EventLoop *loop)
    : loop_(loop),
      epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        std::cout << "Epoll::Epoll" << std::endl;
    }
}

Epoll::~Epoll() {
    close(epollfd_);
}

int64_t Epoll::poll(int timeoutMS, ChannelList* activeChannels) {
    int eventNums = epoll_wait(epollfd_, events_.data(),
            static_cast<int>(events_.size()), timeoutMS);

    if (eventNums > 0) {
        fillActiveChannels(eventNums, activeChannels);
        if (static_cast<EventList::size_type>(eventNums) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (eventNums == 0) {
        std::cout << "nothing happended at Process" << getpid() << std::endl;
    } else {
        std::cout << "Epoll::poll()" << std::endl;
    }
}

void Epoll::updateChannel(Channel *channel) {
    std::cout << "Epoll::updateChannel() fd = " << channel->fd()
              << " event = " << channel->getEvents() << std::endl;

    const int status = channel->getStatusInEpoll();
    int fd = channel->fd();

    if (status == kNew || status == kDeleted) {
        if (status == kNew) {
            // 未在 ChannelMap 中，需要建立映射
            assert(channels_.find(fd) == channels_.end());

            channels_[fd] = channel;
        } else {
            // 之前被删除了，但还存在 ChannelMap 中
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->setStatusInEpoll(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(status == kAdded);

        if (channel->isNoneEvent()) {
            // Channel 没有关心的事件，则要在 epoll 中移除该 Channel 对应的文件描述符
            update(EPOLL_CTL_DEL, channel);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoll::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);
    if (channel->getStatusInEpoll() == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setStatusInEpoll(kNew);
}

bool Epoll::hasChannel(Channel *channel) {
    auto it = channels_.find(channel->fd());

    return it != channels_.end() && it->second == channel;
}

void Epoll::fillActiveChannels(int eventNums, ChannelList *activeChannels) const {
    assert(eventNums <= events_.size());

    for (int i = 0; i < eventNums; ++i) {
        auto* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void Epoll::update(int operation, Channel *channel) {
    epoll_event event{};

    // struct epoll_event {
    //     unit32_t events;
    //     epoll_data_t data;
    // }
    //
    // struct union epoll_data {
    //     void *ptr;
    //     int fd;
    //     uint32_t u32;
    //     uint64_t u64;
    // } epoll_data_t;
    //
    // 使用 ptr 成员保存 channel 指针，
    // 方便 Epoll::fillActiveChannels() 填充 activeChannel 时获取 Channel 对象
    event.events = static_cast<uint32_t>(channel->getEvents());
    event.data.ptr = channel;
    int fd = channel->fd();
    if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        std::cout << "epoll_ctl op=" << operationToString(operation)
                  << " fd=" << fd << std::endl;
    }
}

std::string Epoll::operationToString(int operation) const {
    switch (operation) {
        case EPOLL_CTL_ADD:
            return "epoll add";
        case EPOLL_CTL_DEL:
            return "epoll delete";
        case EPOLL_CTL_MOD:
            return "epoll modify";
        default:
            assert(false); // 对于非法的操作，便于调试阶段发现错误
            return "Unknown Operation";
    }
}