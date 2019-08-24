#ifndef TINYWS_HTTPSERVER_H
#define TINYWS_HTTPSERVER_H

#include <functional>
#include <string>

#include "../base/noncopyable.h"
#include "../net/TcpServer.h"
#include "../net/TcpConnection.h"
#include "../net/Timer.h"

namespace tinyWS{
    class Buffer;
    class HttpRequest;
    class HttpResponse;

    class HttpServer : noncopyable {
    public:
        // HTTP 请求到来时的回调函数的类型
        using HttpCallback = std::function<void(const HttpRequest&, HttpResponse&)>;

        /**
         * 构造函数
         * @param loop 所属 EventLoop
         * @param listenAddress 监听地址
         * @param name TcpServer name
         */
        HttpServer(EventLoop *loop,
                   const InternetAddress& listenAddress,
                   const std::string &name);

        /**
         * 获取所属 EventLoop
         * @return EventLoop
         */
        EventLoop* getLoop() const;

        /**
         * 设置 HTTP 请求到来时的回调函数
         * @param cb
         */
        void setHttpCallback(const HttpCallback &cb);

        /**
         * 设置 IO 线程数
         * @param threadsNum 线程数
         */
        void setThreadNum(int threadsNum);

        /**
         * 启动 TcpServer
         */
        void start();
    private:
        TcpServer tcpServer_;       // TcpServer
        HttpCallback httpCallback_; // HTTP 请求到来时的回调函数

        /**
         * 连接建立后，将 HttpContext 传给 TcpConnection。
         * @param connection TcpConnectionPtr
         */
        void onConnection(const TcpConnectionPtr &connection);

        /**
         * 请求到来后，解析请求，响应请求。
         * @param connection
         * @param buffer
         * @param receiveTime
         */
        void onMessage(const TcpConnectionPtr &connection,
                       Buffer *buffer,
                       Timer::TimeType receiveTime);
        /**
         * 当解析完一条请求信息后，响应请求。
         * @param connection TcpConnectionPtr
         * @param httpRequest
         */
        void onRequest(const TcpConnectionPtr &connection,
                       const HttpRequest &httpRequest);
    };
}

#endif //TINYWS_HTTPSERVER_H
