#include "EventLoop.h"

#include <iostream>

#include "Channel.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "TimerId.h"
#include "status.h"

using namespace tinyWS_process2;

const int kEpollTimeMs = 500;

EventLoop::EventLoop()
    : running_(false),
      epoll_(new Epoll(this)),
      pid_(getpid()),
      timerQueue_(new TimerQueue(this)),
      listenSockfd_(-1) {

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

        if (beforeEachLoopFunction_) {
            beforeEachLoopFunction_();
        }

        activeChannels_.clear();
        auto receiveTime = epoll_->poll(kEpollTimeMs, &activeChannels_);

        Channel* listenSockfdChannel = nullptr;
        for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it) {
            if ((*it)->fd() == listenSockfd_) {
                listenSockfdChannel = *it;
                activeChannels_.erase(it);
                break;
            }
        }

        if (listenSockfdChannel != nullptr) {
//            std::cout << "listenSockfdChannel in process " << getpid() << std::endl;
            listenSockfdChannel->handleEvent(receiveTime);

            if (afterEachAcceptFunction_) {
//                std::cout << "afterEachLoopFunction_ in process " << getpid() << std::endl;
                afterEachAcceptFunction_();
            }
        }


        // stop this loop if get signal SIGINT SIGTERM SIGKILL SIGQUIT SIGCHLD(parent process)
        if (status_quit_softly == 1 || status_terminate == 1) {
            std::cout << "process(" << getpid() << ") quit this eventloop" << std::endl;
            running_ = false;
            break;
        }

        for (auto channel : activeChannels_) {
            channel->handleEvent(receiveTime);
        }

        if (status_child_quit == 1) {
            std::cout << "[SIGCHLD] process(" << getpid() << ") quit this eventloop" << std::endl;
            running_ = false;
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

void EventLoop::setListenSockfd(int sockfd) {
    listenSockfd_ = sockfd;
}

void EventLoop::setBeforeEachLoopFunction(const BeforeEachLoopFunction& cb) {
    beforeEachLoopFunction_ = cb;
}

void EventLoop::setAfterEachLoopFunction(const AfterEachAcceptFunction& cb) {
    afterEachAcceptFunction_ = cb;
}

void EventLoop::printActiveChannels() const {
    for (auto channel : activeChannels_) {
        std::cout << "{" << channel->reventsToString() << "} " << std::endl;
    }
}