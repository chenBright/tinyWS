#include "HttpServer.h"

#include <iostream>

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../net/TimerId.h"

using namespace std::placeholders;
using namespace tinyWS_process2;

HttpServer::HttpServer(const InternetAddress& listenAddress, const std::string& name)
        : tcpServer_(listenAddress, name),
          httpCallback_() {
    tcpServer_.setConnectionCallback(
            std::bind(&HttpServer::onConnection, this, _1));
    tcpServer_.setMessageCallback(
            std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

EventLoop* HttpServer::getLoop() const {
    return tcpServer_.getLoop();
}

void HttpServer::setHttpCallback(const HttpCallback& cb) {
    httpCallback_ = cb;
}

void HttpServer::start() {
    tcpServer_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& connection) {
    if (connection->connected()) {
        connection->setContext(HttpContext());
    }
}

TimerId HttpServer::runAt(TimeType runTime, const Timer::TimerCallback& cb) {
    return tcpServer_.runAt(runTime, cb);
}

TimerId HttpServer::runAfter(TimeType delay, const Timer::TimerCallback& cb) {
    return tcpServer_.runAfter(delay, cb);
}

TimerId HttpServer::runEvery(TimeType interval, const Timer::TimerCallback& cb) {
    return tcpServer_.runEvery(interval, cb);
}

void HttpServer::onMessage(const TcpConnectionPtr& connection, Buffer* buffer,
                           TimeType receiveTime) {
    HttpContext* context = connection->getMutableContext();
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

void HttpServer::onRequest(const TcpConnectionPtr& connection,
                           const HttpRequest& httpRequest) {
    const std::string &connectionStr = httpRequest.getHeader("Connection");
    bool isClose = connectionStr == "close";
    HttpResponse response(isClose);

    if (httpCallback_) {
        httpCallback_(httpRequest, response);
    }

    Buffer buffer;
    response.appendToBuffer(&buffer);
    connection->send(buffer.toString());
    // 如果 isClose 为 true，即将要关闭连接，但是数据还没发送完，连接也会在数据发完才会关闭。
    //  在 TcpConnection::handleWrite() 和
    // TcpConnection::send() 只调用了一次 write(2)而不会反复调用直至它返回 EAGAIN，
    // 因为如果第一次 write(2) 没有能够将数据发送完，第二次调用 write(2) 几乎肯定会将数据发送完，并返回 EAGAIN。
    // 那么第二次调用 write(2) 会发生在连接关闭之前吗？
    // 会。
    // 第一次如果没有将数据发送完，会令 Channel 关注写事件（此时调用 Channel::isWriting() 会返回 true，表示 Channel 处在写数据状态）。
    // 当写事件到来时，调用 TcpConnection::handleWrite()，其中会第二次调用 write(2) 将数据发送完。
    // 而 TcpConnection::shutdown() 只有当对应的 CHannel 不处在写数据状态才会关闭连接，而当对应 Channel 处在写数据状态，则不做任何操作。
    // 所以，数据肯定能发送完，在关闭连接。
    if (response.closeConnection()) {
        connection->shutdown();
    }
}
