#include "EventLoop.h"

#include <iostream>

#include "Epoll.h"
#include "Channel.h"
#include "../base/utility.h" // make_unique

using namespace tinyWS_process;

EventLoop::EventLoop()
    : looping_(false),
      epoll_(make_unique<Epoll>(this)),
      pid_(getpid()){

    std::cout << "EventLoop created "
            << this << " in process "
            << pid_ << std::endl;
}

void EventLoop::loop() {
    looping_ = true;

    std::cout << "EventLoop " << this << "start looping" << std::endl;

    while (looping_) {
        activeChannels_.clear();
        auto receiveTime = epoll_->poll(100, &activeChannels_);

        for (auto channel : activeChannels_) {
            channel->handleEvent(receiveTime);
        }
    }

    looping_ = false;
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