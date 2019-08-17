#ifndef TINYWS_CALLBACK_H
#define TINYWS_CALLBACK_H

#include <functional>
#include <memory>

namespace tinyWS {
    class TcpConnection;

    // TcpConnection 对象的智能指针类型
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

    // 连接建立的回调函数的类型
    typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;

    // 消息到来的回调函数的类型
    typedef std::function<void (const TcpConnectionPtr&, Buffer*, Timer::TimeType)> MessageCallback;

    // 连接断开的的回调函数的类型
    typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;

    // 写完成的回调函数的类型
    typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;

    // "高水位"回调函数的类型
    typedef std::function<void(const TcpConnectionPtr&)> HighWaterMarkCallback;
}


#endif //TINYWS_CALLBACK_H
