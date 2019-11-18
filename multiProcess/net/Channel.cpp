#include "Channel.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include "EventLoop.h"

using namespace tinyWS_process;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fdArg)
    : loop_(loop),
      fd_(fdArg),
      events_(0),
      revents_(0),
      statusInEpoll_(-1) {

}


void Channel::handleEvent(int64_t receiveTime) {
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        std::cout << "Channel::handleEvent() EPOLLHUP" << std::endl;
        if (closeCallback_) {
            closeCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLRDHUP | EPOLLPRI)) {
        if (readCallback_) {
            readCallback_();
        }
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
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

void Channel::update() {
    loop_->updateChannel(this);
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