#include <iostream>
#include <functional>

#include <unistd.h>

#include "EventLoop.h"
#include "base/Thread.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "Timer.h"
#include "TcpServer.h"

using namespace std::placeholders;
using namespace tinyWS;

int threadNums = 0;

class EchoServer {
public:
    EchoServer(EventLoop *loop, const InternetAddress &listenAddress)
        : loop_(loop),
          server_(loop_, listenAddress, "EchoServer") {
        server_.setConnectionCallback(
                std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(
                std::bind(&EchoServer::onMessage, this, _1, _2, _3));
        server_.setThreadNumber(threadNums);
    }

    void start() {
        server_.start();
    }

private:
    EventLoop *loop_;
    TcpServer server_;

    void onConnection(const TcpConnectionPtr &connection) {
        std::cout << connection->peerAddress().toIpPort() << " -> "
            << connection->localAddress().toIpPort() << " is "
            << (connection->connected() ? "UP" : "DOWN") << std::endl;
        connection->send("hello\n");
    }

    void onMessage(const TcpConnectionPtr &connection, Buffer *buffer, Timer::TimeType time) {
        std::string message(buffer->retrieveAllAsString());
        std::cout << connection->name() << " recv " << message.size() << " bytes at " << time << std::endl;
        if (message == "exit\n") {
            connection->send("bye\n");
            connection->shutdown();
        }
        if (message == "quit\n") {
            loop_->quit();
        }
        connection->send(message);
    }

};

EventLoop *g_loop;

void threadFunction() {
    printf("new thread\n");
    g_loop->loop();
}

int main(int argc, char *argv[]) {
//    EventLoop loop;
//    g_loop = &loop;
//    Thread t(threadFunction);
//    t.start();
//    t.join();

    std::cout  << "pid = " << ::getpid() << ", tid = " << Thread::gettid() << std::endl;
    std::cout << "sizeof TcpConnection = " << sizeof(TcpConnection) << std::endl;

    if (argc > 1) {
        threadNums = ::atoi(argv[1]);
    }

    EventLoop loop;
    InternetAddress listenAddress(2000);
    EchoServer server(&loop, listenAddress);

    server.start();

    return 0;
}
