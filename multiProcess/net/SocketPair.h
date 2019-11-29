#ifndef TINYWS_SOCKETPAIR_H
#define TINYWS_SOCKETPAIR_H

#include <string>
#include <memory>

#include "../base/noncopyable.h"
#include "type.h"

namespace tinyWS_process {

    class EventLoop;
    class TcpConnection;

    class SocketPair : noncopyable {
    private:
        EventLoop* loop_;
        int fds_[2];
        std::unique_ptr<TcpConnection> connection_;

        bool isParent_;

    public:
        SocketPair(EventLoop* loop, int fds[2]);

        ~SocketPair();

        void setParentSocket(int port);

        void setChildSocket(int port);

        void writeToChild(const std::string& data);

        void writeToParent(const std::string& data);

        void setConnectCallback(const ConnectionCallback& cb);

        void setMessageCallback(const MessageCallback& cb);

        void setWriteCompleteCallback(const WriteCompleteCallback& cb);

        void setCloseCallback(const CloseCallback& cb);

        void clearSocket();

    };

}

#endif //TINYWS_SOCKETPAIR_H
