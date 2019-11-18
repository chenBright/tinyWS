//
// Created by 陈光明 on 2019/11/17.
//

#ifndef TINYWS_EVENTLOOP_H
#define TINYWS_EVENTLOOP_H

#include <unistd.h>

#include <vector>
#include <memory>

namespace tinyWS_process {
    class Channel;
    class Epoll;

    class EventLoop {
    private:
        using ChannelList = std::vector<Channel*>;

        bool looping_;
        std::unique_ptr<Epoll> epoll_;
        pid_t pid_;
        ChannelList activeChannels_;

    public:
        EventLoop();

        ~EventLoop() = default;

        void loop();

        void quit();

//        void runAt();
//
//        void runAfter();
//
//        void runEvery();

        void updateChannel(Channel* channel);

        void removeChannel(Channel* channel);

    private:
        void printActiveChannels() const; // for DEBUG
    };
}


#endif //TINYWS_EVENTLOOP_H
