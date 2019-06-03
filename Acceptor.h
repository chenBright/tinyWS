#ifndef TINYWS_ACCEPTOR_H
#define TINYWS_ACCEPTOR_H

#include <functional>

#include "base/noncopyable.h"
#include "Channel.h"
#include "Socket.h"

namespace tinyWS {
    class InternetAddress;
    class EventLoop;

    /**
     * 内部类，供 TcpServer 使用，生命周期有 TcpServer 控制
     * 用于 accept(2) 新 TCP 连接，并通过回调函数通知使用者
     */
    class Acceptor : noncopyable {
    public:
        typedef std::function<void(Socket, const InternetAddress&)> NewConnectionCallback;

        Acceptor(EventLoop *loop, const InternetAddress &listenAdress);
        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback &cb);

        bool isListening() const;
        void listen();

        static int createNonblocking();

    private:
        EventLoop *loop_; // 所属 EventLoop
        Socket acceptSocket_; // listening socket
        Channel acceptChannel_; // 用于观察 acceptSocket_ 的 readable 事件
        NewConnectionCallback newConnectionCallback_;
        bool isListening_;

        void hadleRead();
    };
}


#endif //TINYWS_ACCEPTOR_H
