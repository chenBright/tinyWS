#include "Channel.h"

#include <sys/epoll.h>

#include <iostream>
#include <sstream>

#include "EventLoop.h"

using namespace tinyWS;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fdArg)
    : loop_(loop),
      fd_(fdArg),
      events_(0),
      revents_(0),
      statusInEpoll_(-1),
      eventHandling_(false) {
}

Channel::~Channel() {
    // 事件处理期间，不会析构
    assert(!eventHandling_);
    std::cout << "Channel:~Channel()" << std::endl;
}

// TODO 处理 receiveTime
void Channel::handleEvent(Timer::TimeType receiveTime) {
    eventHandling_ = true;

    // 位操作的的编译器警告：Use of a signed integer operand with a binary bitwise operator
    // 参考 https://succlz123.wordpress.com/2018/04/12/undefined-behavior-warning-in-c/

    // 连接断开事件
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
//    if (revents_ & EPOLLHUP) {
        std::cout << "Channel::handleEvent() EPOLLHUP" << std::endl;
        if (closeCallback_) {
            closeCallback_();
        }
    }

    // 可读事件
    if (revents_ & EPOLLIN) {
        std::cout << "Channel::handleEvent() EPOLLIN" << std::endl;
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

void Channel::setReadCallback(const ReadEventCallback &cb) {
    readCallback_ = cb;
}

void Channel::setWriteCallback(const Channel::EventCallback &cb) {
    writeCallback_ = cb;
}
void Channel::setCloseCallback(const Channel::EventCallback &cb) {
    closeCallback_ = cb;
}

void Channel::setErrorCallback(const Channel::EventCallback &cb) {
    errorCallback_ = cb;
}

int Channel::fd() const {
    return fd_;
}

int Channel::getEvents() const {
    return events_;
}

void Channel::setRevents(int revt) {
    revents_ = revt;
}

bool Channel::isNoneEvent() const {
    return revents_ == kNoneEvent;
}

void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
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

EventLoop* Channel::ownerLoop() {
    return loop_;
}

void Channel::remove() {
    loop_->removeChannel(this);
}

std::string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

void Channel::update() {
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

