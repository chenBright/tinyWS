#ifndef TINYWS_CALLBACK_H
#define TINYWS_CALLBACK_H

#include <functional>
#include <memory>

namespace tinyWS_thread {
    class TcpConnection;

    // TcpConnection 对象的智能指针类型
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    // 连接建立的回调函数的类型
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

    // 消息到来的回调函数的类型
    using MessageCallback = std::function<void (const TcpConnectionPtr&, Buffer*, Timer::TimeType)>;

    // 连接断开的的回调函数的类型
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

    // 写完成的回调函数的类型
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

    // "高水位"回调函数的类型
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&)>;
}


#endif //TINYWS_CALLBACK_H
