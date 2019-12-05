#include "EventLoop.h"

#include <iostream>

#include "Channel.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "TimerId.h"
#include "status.h"

using namespace tinyWS_process;

const int kEpollTimeMs = 10000;

EventLoop::EventLoop()
    : running_(false),
      epoll_(new Epoll(this)),
      pid_(getpid()),
      timerQueue_(new TimerQueue(this)) {

    std::cout << "EventLoop created "
              << this << " in process "
              << pid_ << std::endl;
}

EventLoop::~EventLoop() {

}

void EventLoop::loop() {
    running_ = true;

    std::cout << "EventLoop " << this << " create looping" << std::endl;

    while (running_) {
        activeChannels_.clear();
        auto receiveTime = epoll_->poll(kEpollTimeMs, &activeChannels_);

        for (auto channel : activeChannels_) {
            channel->handleEvent(receiveTime);
        }
    }
}

void EventLoop::quit() {
    running_ = false;
}

TimerId EventLoop::runAt(TimeType runTime, const Timer::TimerCallback& cb) {
    return timerQueue_->addTimer(cb, runTime, 0);
}

TimerId EventLoop::runAfter(TimeType delay, const Timer::TimerCallback& cb) {
    return runAt(Timer::now() + delay, cb);
}

TimerId EventLoop::runEvery(TimeType interval, const Timer::TimerCallback& cb) {
    std::cout << timerQueue_.get() << std::endl;
    return timerQueue_->addTimer(cb, Timer::now(), interval);
}

void EventLoop::cancel(const TimerId& timerId) {
    timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel *channel) {
    epoll_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    epoll_->removeChannel(channel);
}


void EventLoop::printActiveChannels() const {
    for (auto channel : activeChannels_) {
        std::cout << "{" << channel->reventsToString() << "} " << std::endl;
    }
}