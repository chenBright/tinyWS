#ifndef TINYWS_CALLBACK_H
#define TINYWS_CALLBACK_H

#include <functional>
#include <memory>

namespace tinyWS {
    class TcpConnection;

    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&, Buffer*, Timer::TimeType)> MessageCallback;
    typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
    typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void(const TcpConnectionPtr&)> HighWaterMarkCallback;
}


#endif //TINYWS_CALLBACK_H
