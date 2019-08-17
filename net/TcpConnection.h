#ifndef TINYWS_TCPCONNECTION_H
#define TINYWS_TCPCONNECTION_H

#include <functional>
#include <memory>
#include <string>

#include "../base/noncopyable.h"
#include "Timer.h"
#include "InternetAddress.h"
#include "Buffer.h"
#include "CallBack.h"
#include "../http/HttpContext.h"

namespace tinyWS {
    class EventLoop;
    class Socket;
    class Channel;

    // 由于 TcpConnection 模糊的生命周期，所以需要继承自 enable_shared_from_this。
    // TODO 原因见 《Linux多线程服务端编程》P100
    //
    // TcpConnection 表示的是"一次 TCP 连接"，它是不可再生的，
    // 一旦连接断开，这个 TcpConnection 对象就没有用了。
    // TcpConnection 没有发起连接的功能，其构造函数的参数是已经建立好连接的 socket 对象，
    // 因此其初始状态是 kConnecting。
    class TcpConnection : noncopyable,
                          public std::enable_shared_from_this<TcpConnection> {
    public:
        /**
         * 构造函数
         * @param loop 所属 EventLoop
         * @param name 连接名
         * @param socket socket 对象
         * @param localAddress 本地地址对象
         * @param peerAddress 客户端地址对象
         */
        explicit TcpConnection(EventLoop *loop,
                               const std::string &name,
                               Socket socket,
                               const InternetAddress &localAddress,
                               const InternetAddress &peerAddress);
        ~TcpConnection();

        /**
         * 发送数据
         * @param message 数据
         */
        void send(const std::string &message);

        /**
         * 发送数据
         * @param message 数据字符串指针
         * @param len 需要发送数据的长度
         */
        void send(const void *message, size_t len);

        /**
         * shutdown write 端
         * 只有处于 kConnected 状态才能 shutdown，转换成 kDisconnecting 状态。
         */
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

        /**
         * 设置连接建立回调函数
         * @param cb 回调函数
         */
        void setConnectionCallback(const ConnectionCallback &cb);

        /**
         * 设置消息读取成功回调函数
         * @param cb 回调函数
         */
        void setMessageCallback(const MessageCallback &cb);

        /**
         * 设置连接断开回调函数
         * @param cb 回调函数
         */
        void setCloseCallback(const CloseCallback &cb);

        /**
         * 设置写完成回调函数
         * @param cb 回调函数
         */
        void setWriteCompleteCallback(const WriteCompleteCallback &cb);

        /**
         * 设置高水位回调函数
         * @param cb 回调函数
         * @param highWaterMark 水位值
         */
        void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark);

        /**
         * TcpServer 接受到一个连接时调用该函数，
         * 用于设置 TcpConnection 状态、设置 Channel 可读和调用 connection callback。
         */
        void connectionEstablished();

        /**
         * 该函数是 TcpConnection 析构前调用的最后一个成员函数，用于通知用户连接已断开。
         * TcpServer 将 TcpConnection 从 ConnectionMap 中移除后调用该函数，
         * 用于设置 TcpConnection 状态、设置 Channel 全部事件不可用和调用 connection callback。
         */
        void connectionDestroyed();

        std::string name();

        EventLoop* getLoop() const;

        const InternetAddress& localAddress() const;
        const InternetAddress& peerAddress() const;

        bool connected() const;
        bool disconnected() const;


    private:
        // 状态转移图可见《Linux多线程服务端编程》P317
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

        EventLoop *loop_;                               // 所属 EventLoop
        std::string name_;                              // 连接名
        StateE state_;                                  // 当前连接状态
        std::unique_ptr<Socket> socket_;                // Socket 对象的只能指针，为 TcpConnection 独享
        // TODO 考虑清楚 channel_ 的所有权，loop 中的 Channel ？
        std::unique_ptr<Channel> channel_;              // socket fd 的 Channel 对象的只能指针，为 TcpConnection 独享
        InternetAddress localAddress_;                  // 本地地址对象
        InternetAddress peerAddress_;                   // 客户端地址对象
        Buffer inputBuffer_;                            // 输入缓冲区
        Buffer outputBuffer_;                           // 输出缓冲区
        HttpContext context_;                           // 接收到的请求的内容

        ConnectionCallback connectionCallback_;         // 连接建立回调函数
        MessageCallback messageCallback_;               // 消息读取成功回调函数
        CloseCallback closeCallback_;                   // 连接断开回调函数
        WriteCompleteCallback writeCompleteCallback_;   // 写完成回调函数，在 sendInLoop、handleWrite 中调用
        HighWaterMarkCallback highWaterMarkCallback_;   // TODO 未实现"高水位回调"功能
        size_t highWaterMark_;

        /**
         * 设置连接状态
         * @param s 状态
         */
        void setState(StateE s);

        // TcpConnection 的一系列 handle* 函数会在 TcpConnection 构造函数中设置为 Channel 对应的回调函数。
        // 所以 TcpConnection 的 handle 动作由 Channel 调用回调函数导致的。

        /**
         * 从 Buffer 中读数据
         * 如果读出的字节数大于 0，则调用 message callback；
         * 如果读出的字节数为 0，则断开连接；
         * 如果读出的字节数小于 0，则出错，调用错误处理函数。
         * @param receiveTime
         */
        void handleRead(Timer::TimeType receiveTime);

        /**
         * 写数据
         */
        void handleWrite();

        /**
         * 断开连接
         */
        void handleClose();

        /**
         * 处理连接异常
         * 只是在日志中打印错误信息，这不影响连接的正常关闭。
         */
        void handleError();

        /**
         * --- 安全线程 ---
         * 在 IO 线程中发送数据
         * @param message
         */
        void sendInLoop(const std::string &message);
//        void sendInLoop(const void *message, size_t len);

        /**
         * 在 IO 线程中 shutdown write 端
         */
        void shutdownInLoop();
    };
}

#endif //TINYWS_TCPCONNECTION_H
