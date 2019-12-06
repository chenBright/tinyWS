#ifndef TINYWS_ACCEPTOR_H
#define TINYWS_ACCEPTOR_H

#include <functional>

#include "../base/noncopyable.h"
#include "Channel.h"
#include "Socket.h"

namespace tinyWS_process2 {

    class InternetAddress;
    class EventLoop;

    // 内部类，供 TcpServer 使用，生命周期由 TcpServer 控制。
    // 用于 accept(2) 新 TCP 连接，并通过回调函数通知使用者。
    class Acceptor : noncopyable {
    public:
        using NewConnectionCallback = std::function<void(Socket, const InternetAddress)>;

    private:
        EventLoop* loop_;
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool isListening_;

    public:
        Acceptor(EventLoop* loop, const InternetAddress& listenAddress);

        Acceptor(EventLoop* loop, Socket socket, const InternetAddress& listenAddress);

        ~Acceptor();

        void resetLoop(EventLoop* loop);

        void setNewConnectionCallback(const NewConnectionCallback& cb);

        void listen();

        void unlisten();

        bool isLIstening() const;

        static int createNonblocking();

    private:
        void handleRead();
    };
}


#endif //TINYWS_ACCEPTOR_H
