#include "Channel.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>

#include <iostream>
#include <sstream>

#include "EventLoop.h"

using namespace tinyWS_process2;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fdArg)
    : loop_(loop),
      fd_(fdArg),
      events_(kNoneEvent),
      revents_(kNoneEvent),
      statusInEpoll_(-1),
      eventHandling_(false),
      addedToLoop_(false),
      tied_(false) {

}

Channel::~Channel() {
    // 事件处理期间，不会析构
    assert(!eventHandling_);
    assert(!addedToLoop_);
}

void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent(TimeType receiveTime) {
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::setReadCallback(const ReadEventCallback &cb) {
    readCallback_ = cb;
}

void Channel::setWriteCallack(const EventCallback &cb) {
    writeCallback_ = cb;
}

void Channel::setCloseCallback(const EventCallback &cb) {
    closeCallback_ = cb;
}

void Channel::setErrorCallback(const EventCallback &cb) {
    errorCallback_ = cb;
}

int Channel::fd() const {
    return fd_;
}

int Channel::getEvents() const {
    return events_;
}

int Channel::setRevents(int revt) {
    revents_ = revt;
}

bool Channel::isNoneEvent() const {
    return events_ == kNoneEvent;
}

void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::disableReading() {
    events_ &= ~kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

bool Channel::isWriting() const {
    return static_cast<bool>(events_ & kWriteEvent);
}

int Channel::getStatusInEpoll() {
    return statusInEpoll_;
}

void Channel::setStatusInEpoll(int statusInEpoll) {
    statusInEpoll_ = statusInEpoll;
}

void Channel::resetLoop(EventLoop* loop) {
    loop_ = loop;

    events_ = kNoneEvent;
    revents_ = kNoneEvent;
    statusInEpoll_ = -1;
    eventHandling_ = false;
    addedToLoop_ = false;
    tied_ = false;
}

EventLoop* Channel::ownerLoop() {
    return loop_;
}

void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

std::string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

void Channel::handleEventWithGuard(TimeType receiveTime) {
    eventHandling_ = true;

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
//        std::cout << "Channel::handleEvent() EPOLLHUP" << std::endl;
        if (closeCallback_) {
            closeCallback_();
        }
    }

    // 可读事件
    if (revents_ & EPOLLIN) {
//        std::cout << "Channel::handleEvent() EPOLLIN" << std::endl;
    }

    // 异常事件
    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    // 可读事件
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }

    // 可写事件
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }

    eventHandling_ = false;
}

void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

std::string Channel::eventsToString(int fd, int event) const {
    std::ostringstream oss;
    oss << fd << ": ";
    if (event & EPOLLIN) {
        oss << "IN ";
    }
    if (event & EPOLLPRI) {
        oss << "PRI ";
    }
    if (event & EPOLLOUT) {
        oss << "OUT ";
    }
    if (event & EPOLLHUP) {
        oss << "HUP ";
    }
    if (event & EPOLLRDHUP) {
        oss << "RDHUP ";
    }
    if (event & EPOLLERR) {
        oss << "ERR ";
    }

    return oss.str();
}