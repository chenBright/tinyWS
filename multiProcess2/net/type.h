#ifndef TINYWS_TYPE_H
#define TINYWS_TYPE_H

#include <cstdint>
#include <functional>
#include <memory>

namespace tinyWS_process2 {
    // 定时器中的时间类型
    using TimeType = int64_t;

    // TcpConnection 相关的类型
    class TcpConnection;
    class Buffer;

    // TcpConnection 对象的智能指针类型
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    // 连接建立的回调函数的类型
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

    // 消息到来的回调函数的类型
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimeType)>;

    // 连接断开的的回调函数的类型
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

    // 写完成的回调函数的类型
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
}

#endif //TINYWS_TYPE_H
