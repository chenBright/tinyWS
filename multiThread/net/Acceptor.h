#ifndef TINYWS_ACCEPTOR_H
#define TINYWS_ACCEPTOR_H

#include <functional>

#include "../base/noncopyable.h"
#include "Channel.h"
#include "Socket.h"

namespace tinyWS_thread {
    class InternetAddress;
    class EventLoop;

    // 内部类，供 TcpServer 使用，生命周期由 TcpServer 控制。
    // 用于 accept(2) 新 TCP 连接，并通过回调函数通知使用者。
    class Acceptor : noncopyable {
    public:
        // 新连接到来时的回调函数的类型
        using NewConnectionCallback = std::function<void(Socket, const InternetAddress&)>;

        /**
         * 构造函数
         * @param loop 所属 EventLoop
         * @param listenAdress 监听的地址
         */
        Acceptor(EventLoop *loop, const InternetAddress &listenAdress);

        ~Acceptor();

        /**
         * 设置新连接到来时的回调函数
         * @param cb 回调函数
         */
        void setNewConnectionCallback(const NewConnectionCallback &cb);

        /**
         * 是否处于监听状态
         * @return true / false
         */
        bool isListening() const;

        /**
         * 监听
         */
        void listen();

        /**
         * 创建无阻塞的 socket
         * @return socket
         */
        static int createNonblocking();

    private:
        EventLoop *loop_;                               // 所属 EventLoop
        Socket acceptSocket_;                           // listening socket
        Channel acceptChannel_;                         // 用于观察 acceptSocket_ 的 readable 事件
        NewConnectionCallback newConnectionCallback_;   // 新连接到来时的回调函数
        bool isListening_;                              // 是否处于监听状态

        /**
         * 读数据，获取新连接对应的 socket fd，并调用新连接到来时的回调函数，停止用户有新的连接。
         */
        void hadleRead();
    };
}


#endif //TINYWS_ACCEPTOR_H
