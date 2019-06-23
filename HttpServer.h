#ifndef TINYWS_HTTPSERVER_H
#define TINYWS_HTTPSERVER_H

#include <functional>
#include <string>

#include "base/noncopyable.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "Timer.h"

namespace tinyWS{
    class Buffer;
    class HttpRequest;
    class HttpResponse;

    class HttpServer : noncopyable {
    public:
        typedef std::function<void(const HttpRequest&,
                HttpResponse&)> HttpCallback;

        HttpServer(EventLoop *loop,
                   const InternetAddress& listenAddress,
                   const std::string &name);
        EventLoop* getLoop() const;
        void setHttpCallback(const HttpCallback &cb);

        void setThreadNum(int threadsNum);
        void start();
    private:
        TcpServer tcpServer_;
        HttpCallback httpCallback_;

        void onConnection(const TcpConnectionPtr &connection);
        void onMessage(const TcpConnectionPtr &connection,
                       Buffer *buffer,
                       Timer::TimeType receiveTime);
        /**
         * 当解析完一条请求信息后，响应请求
         * @param connection
         * @param httpRequest
         */
        void onRequest(const TcpConnectionPtr &connection,
                       const HttpRequest &httpRequest);
    };
}

#endif //TINYWS_HTTPSERVER_H
