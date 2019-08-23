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
        // 404
        connection->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        connection->shutdown();
    }

    // 解析失败 跟 解析完成 互斥

    if (context->gotAll()) {
        // 解析完成，响应请求
        onRequest(connection, context->request());
        // 重置 HttpContext
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
    connection->send(buffer.toString());
    // TODO 如果 isClose 为 true，即将要关闭连接，但是数据还没发送完，将会怎样？
    //  在 TcpConnection::handleWrite() 和 TcpConnection::shutdown()
    if (response.closeConnection()) {
        connection->shutdown();
    }
}
