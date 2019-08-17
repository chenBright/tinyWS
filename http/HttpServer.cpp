#include "HttpServer.h"

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

using namespace std::placeholders;
using namespace tinyWS;

HttpServer::HttpServer(EventLoop *loop,
                       const InternetAddress &listenAddress,
                       const std::string &name)
                       : tcpServer_(loop, listenAddress, name),
                         httpCallback_() {
    tcpServer_.setConnectionCallback(
            std::bind(&HttpServer::onConnection, this, _1));
    tcpServer_.setMessageCallback(
            std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

EventLoop* HttpServer::getLoop() const {
    return tcpServer_.getLoop();
}

void HttpServer::setHttpCallback(const HttpCallback &cb) {
    httpCallback_ = cb;
}

void HttpServer::setThreadNum(int threadsNum) {
    tcpServer_.setThreadNumber(threadsNum);
}

void HttpServer::start() {
    tcpServer_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &connection) {
    if (connection->connected()) {
        connection->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &connection, Buffer *buffer,
                           Timer::TimeType receiveTime) {
    HttpContext *context = connection->getMutableContext();
    if (!context->parseRequest(buffer, receiveTime)) {
        connection->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        connection->shutdown();
    }

    if (context->gotAll()) {
        onRequest(connection, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr &connection,
                           const HttpRequest &httpRequest) {
    const std::string &connectionStr = httpRequest.getHeader("Connection");
    bool isClose = connectionStr == "close";
    HttpResponse response(isClose);

    if (httpCallback_) {
        httpCallback_(httpRequest, response);
    }

    Buffer buffer;
    response.appendToBuffer(&buffer);
    // TODO muduo 中有一个接收 Buffer 参数的重载版本 send 函数
    connection->send(buffer.toString());
    if (response.closeConnection()) {
        connection->shutdown();
    }
}
