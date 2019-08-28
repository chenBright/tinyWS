#include "Epoll.h"

#include <assert.h>
#include <unistd.h> // close
#include <cstring>
#include <sys/epoll.h>

#include <iostream>

#include "Channel.h"

using namespace tinyWS;

// channel 的状态
// 表示还没添加到 ChannelMap 中
const int kNew = -1;
// 已添加到 ChannelMap 中
const int kAdded = 1;
// 无关心事件，已从 epoll 中删除相应文件描述符，但 ChannelMap 中有记录
const int kDeleted = 2;

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
    // 往 events_ 的内存里写入活跃的事件。
    // events_.data() 返回 vector 底层数组的指针，
    // 也可以通过 &*event_.begin() 获取
    int eventNums = epoll_wait(epollfd_, events_.data(),
            static_cast<int>(events_.size()), timeoutMs);
    // TODO 此处暴露了 Timer
    Timer::TimeType now = Timer::now();

    if (eventNums > 0) {
        std::cout << eventNums << " events happen" << std::endl;
        fillActiveChannels(eventNums, activeChannels);
        // 当向epoll中注册的事件过多，导致返回的活动事件可能越來越多，
        // events_ 裝不下时，events_ 扩容为2倍
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
        // 新的文件描述符
        channel->setStatusInEpoll(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(status == kAdded);

        if (channel->isNoneEvent()) {
            // Channel 没有关心的事件，则要在 epoll 中移除该 Channel 对应的文件描述符
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
    channels_.erase(fd); // 删除文件描述符和 Channel 之间的映射关系
    // 如果 epoll 正在监听该文件描述符，则取消 epoll 对该文件描述符的监听。
    if (channel->getStatusInEpoll() == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setStatusInEpoll(kNew); // 设置 Channel 状态为 kNew
}

bool Epoll::hasChannel(tinyWS::Channel *channel) {
    assertInLoopThread();
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

void Epoll::assertInLoopThread() {
    ownerLoop_->assertInLoopThread();
}

void Epoll::fillActiveChannels(int numEvents, Epoll::ChannelList *activeChannels) const {
    assert((size_t) numEvents <= events_.size());

    for (int i = 0; i < numEvents; ++i) {
        // 在 Epoll::update() 中，
        // 使用 epoll_event.data.ptr  保存文件描述符对应的 Channel 对象的指针
        auto *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevents(events_[i].events); // 设置"到来"的事件
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
            << " fd=" << fd;
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