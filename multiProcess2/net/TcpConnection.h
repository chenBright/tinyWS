#ifndef TINYWS_TCPCONNECTION_H
#define TINYWS_TCPCONNECTION_H

#include <functional>
#include <memory>
#include <string>

#include "../base/noncopyable.h"
#include "Timer.h"
#include "InternetAddress.h"
#include "Buffer.h"
#include "type.h"
#include "../base/any.h"

namespace tinyWS_process2 {
    class EventLoop;
    class Socket;
    class Channel;

    // 由于 TcpConnection 模糊的生命周期，所以需要继承自 enable_shared_from_this。
    // 原因见 《Linux多线程服务端编程》P101
    //
    // TcpConnection 表示的是"一次 TCP 连接"，它是不可再生的，
    // 一旦连接断开，这个 TcpConnection 对象就没有用了。
    // TcpConnection 没有发起连接的功能，其构造函数的参数是已经建立好连接的 socket 对象，
    // 因此其初始状态是 kConnecting。
    class TcpConnection : noncopyable,
                          public std::enable_shared_from_this<TcpConnection> {
    private:
        // 状态转移图可见《Linux多线程服务端编程》P317
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

        EventLoop* loop_;
        std::string name_;
        StateE state_;
        std::unique_ptr<Socket> socket_;

        // socket fd 的 Channel 对象的智能指针，为 TcpConnection 独享。
        // 虽然 Epoll 对象持有 Channel 的指针，但已确保在连接关闭时，
        // 调用 TcpConnection::connectionDestroyed()，
        // 转而调用 Channel::remove() 来移除 Epoll 中持有的 Channel 指针。
        // 当有事件到来时，IO 线程被唤醒，EventLoop 从 Epoll 中获得 Channel 的指针。
        // 但 EventLoop 只是短暂持有，在下一事件循环之前，持有的 Channel 指针已被销毁
        std::unique_ptr<Channel> channel_;

        InternetAddress localAddress_;                  // 本地地址对象
        InternetAddress peerAddress_;                   // 客户端地址对象
        Buffer inputBuffer_;                            // 输入缓冲区
        Buffer outputBuffer_;                           // 输出缓冲区
        tinyWS_process2::any context_;                  // 接收到的请求的内容

        ConnectionCallback connectionCallback_;         // 连接建立回调函数
        MessageCallback messageCallback_;               // 消息读取成功回调函数
        CloseCallback closeCallback_;                   // 连接断开回调函数
        WriteCompleteCallback writeCompleteCallback_;   // 写完成回调函数，在 send、handleWrite 中调用

    public:
        explicit TcpConnection(EventLoop* loop,
                               std::string &name,
                               Socket socket,
                               const InternetAddress& localAddress,
                               const InternetAddress& peerAddress);

        ~TcpConnection();

        void send(const std::string& message);

        void send(const void* message, size_t len);

        void shutdown();

        void setContext(const tinyWS_process2::any &context);
        const tinyWS_process2::any& getContext() const;
        tinyWS_process2::any* getMutableContext();

        void setTcpNoDelay(bool on);

        void setKeepAlive(bool on);

        /**
         * 设置连接建立回调函数
         * @param cb 回调函数
         */
        void setConnectionCallback(const ConnectionCallback& cb);

        /**
         * 设置消息读取成功回调函数
         * @param cb 回调函数
         */
        void setMessageCallback(const MessageCallback& cb);

        /**
         * 设置连接断开回调函数
         * @param cb 回调函数
         */
        void setCloseCallback(const CloseCallback& cb);

        /**
         * 设置写完成回调函数
         * @param cb 回调函数
         */
        void setWriteCompleteCallback(const WriteCompleteCallback& cb);

        void connectionEstablished();

        void connectionDestroyed();

        std::string name();

        EventLoop* getLoop() const;

        const InternetAddress& localAddress() const;

        const InternetAddress& peerAddress() const;

        bool connected() const;

        bool disconnected() const;

    private:
        void setState(StateE state);

        void handleRead(TimeType receiveTime);

        void handleWrite();

        void handleClose();

        void handleError();

        void shutdownInSocket();

        /**
         * 获取状态字符串信息。
         * @return
         */
        std::string stateToString() const;
    };

}

#endif //TINYWS_TCPCONNECTION_H
