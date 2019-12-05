#ifndef TINYWS_HTTPSERVER_H
#define TINYWS_HTTPSERVER_H


#include <functional>
#include <string>

#include "../base/noncopyable.h"
#include "../net/TcpServer.h"
#include "../net/TcpConnection.h"
#include "../net/Timer.h"
#include "../net/type.h"

namespace tinyWS_process {

    class Buffer;
    class HttpRequest;
    class HttpResponse;
    class TimerId;

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
        HttpServer(const InternetAddress &listenAddress, const std::string &name);

        /**
         * 获取所属 EventLoop
         * @return EventLoop
         */
        EventLoop* getLoop() const;

        /**
         * 设置 HTTP 请求到来时的回调函数
         * @param cb
         */
        void setHttpCallback(const HttpCallback& cb);

        /**
         * 设置 IO 线程数
         * @param processNum 线程数
         */
        void setProcessNum(int processNum);

        /**
         * 启动 TcpServer
         */
        void start();

        TimerId runAt(TimeType runTime, const Timer::TimerCallback& cb);

        TimerId runAfter(TimeType delay, const Timer::TimerCallback& cb);

        TimerId runEvery(TimeType interval, const Timer::TimerCallback& cb);

    private:
        TcpServer tcpServer_;       // TcpServer
        HttpCallback httpCallback_; // HTTP 请求到来时的回调函数

        /**
         * 连接建立后，将 HttpContext 传给 TcpConnection。
         * @param connection TcpConnectionPtr
         */
        void onConnection(const TcpConnectionPtr& connection);

        /**
         * 请求到来后，解析请求，响应请求。
         * @param connection
         * @param buffer
         * @param receiveTime
         */
        void onMessage(const TcpConnectionPtr& connection,
                       Buffer* buffer,
                       TimeType receiveTime);
        /**
         * 当解析完一条请求信息后，响应请求。
         * @param connection TcpConnectionPtr
         * @param httpRequest
         */
        void onRequest(const TcpConnectionPtr& connection,
                       const HttpRequest& httpRequest);
    };

}

#endif //TINYWS_HTTPSERVER_H
