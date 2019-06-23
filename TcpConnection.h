#ifndef TINYWS_TCPCONNECTION_H
#define TINYWS_TCPCONNECTION_H

#include <functional>
#include <memory>
#include <string>

#include "base/noncopyable.h"
#include "Timer.h"
#include "InternetAddress.h"
#include "Buffer.h"
#include "CallBack.h"
#include "HttpContext.h"

namespace tinyWS {
    class EventLoop;
    class Socket;
    class Channel;

    class TcpConnection : noncopyable,
                          public std::enable_shared_from_this<TcpConnection> {
    public:
        explicit TcpConnection(EventLoop *loop,
                               const std::string &name,
                               Socket socket,
                               const InternetAddress &localAddress,
                               const InternetAddress &peerAddress);
        ~TcpConnection();

        void send(const std::string &message);
        void send(const void *message, size_t len);
        void shutdown();

        // TODO moduo 使用 boost::any，因为 muduo 不单单是实现 HTTP 服务器（而且 TcpConnection 也不应该知道 HttpConnection ？）
        void setContext(const HttpContext &context);
        const HttpContext& getContext() const;
        HttpContext* getMutableContext();

        /**
         * 禁用 Nagle 算法
         * @param on 是否禁用
         */
        void setTcpNoDelay(bool on);

        /**
         * 设置 keep alive
         * @param on 是否禁用
         */
        void setKeepAlive(bool on);

        void setConnectionCallback(const ConnectionCallback &cb);
        void setMessageCallback(const MessageCallback &cb);
        void setCloseCallback(const CloseCallback &cb);
        void setWriteCompleteCallback(const WriteCompleteCallback &cb);
        void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark);

        void connectionEstablished();
        void connectionDestoryed();

        std::string name();

        EventLoop* getLoop() const;

        const InternetAddress& localAddress() const;
        const InternetAddress& peerAddress() const;

        bool connected() const;
        bool disconnected() const;


    private:
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

        EventLoop *loop_;
        std::string name_;
        StateE state_;
        std::unique_ptr<Socket> socket_;
        // TODO 考虑清楚 channel_ 的所有权，loop 中的 Channel ？
        std::unique_ptr<Channel> channel_;
        InternetAddress localAddress_;
        InternetAddress peerAddress_;
        Buffer inputBuffer_;
        Buffer outputBuffer_;
        HttpContext context_; // 接收到的请求的内容

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        CloseCallback closeCallback_;
        WriteCompleteCallback writeCompleteCallback_; // 写完成回调函数，在 sendInLoop、handleWrite 中调用
        HighWaterMarkCallback highWaterMarkCallback_; // TODO 未实现"高水位回调"功能
        size_t highWaterMark_;

        void setState(StateE s);
        void handleRead(Timer::TimeType receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();

        void sendInLoop(const std::string &message);
//        void sendInLoop(const void *message, size_t len);
        void shutdownInLoop();
    };

    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
}

#endif //TINYWS_TCPCONNECTION_H
