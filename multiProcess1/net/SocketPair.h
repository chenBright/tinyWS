#ifndef TINYWS_SOCKETPAIR_H
#define TINYWS_SOCKETPAIR_H

#include <string>
#include <memory>
#include <functional>

#include "../base/noncopyable.h"
#include "type.h"
#include "Socket.h"

namespace tinyWS_process1 {

    class EventLoop;
    class TcpConnection;
    class Channel;
    class Socket;
    class InternetAddress;

    class SocketPair : noncopyable {
    public:
        using ReceiveFdCallback = std::function<void(int)>;
        using CloseCallback = std::function<void()>;

    private:
        EventLoop* loop_;
        int fds_[2];
        std::unique_ptr<TcpConnection> connection_;
        std::unique_ptr<Channel> pipeChannel_;

        bool isParent_;

        ReceiveFdCallback receiveFdCallback_;
        CloseCallback closeCallback_;

    public:
        SocketPair(EventLoop* loop, int fds[2]);

        ~SocketPair();
//
//        SocketPair(SocketPair&& other) noexcept;
//
//        SocketPair& operator=(SocketPair&& other) noexcept;

        void setParentSocket();

        void setChildSocket();

//        void clear();

        void sendFdToChild(Socket socket);

        void sendFdToParent(Socket socket);

        void setReceiveFdCallback(const ReceiveFdCallback& cb);

        void setCloseCallback(const CloseCallback& cb);

    private:
        void sendFd(Socket socket);

        int receiveFd();

        void handleRead();
    };

}

#endif //TINYWS_SOCKETPAIR_H
