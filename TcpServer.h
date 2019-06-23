#ifndef TINYWS_TCPSERVER_H
#define TINYWS_TCPSERVER_H

#include <functional>
#include <memory>
#include <string>
#include <map>

#include "base/noncopyable.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"
#include "CallBack.h"

namespace tinyWS {
    class EventLoop;
    class Acceptor;
    class InternetAddress;

    class TcpServer : noncopyable,
                      std::enable_shared_from_this<TcpServer> {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        TcpServer(tinyWS::EventLoop *loop, const tinyWS::InternetAddress &address, const std::string &name);
        ~TcpServer();

        EventLoop* getLoop() const;

        void setThreadNumber(int threadNumber);
        void start();

        void setConnectionCallback(const ConnectionCallback &cb);
        void setMessageCallback(const MessageCallback &cb);
        void setThreadInitCallback(const ThreadInitCallback &cb);


    private:
        typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

        EventLoop *loop_; // Accept EventLoop
        const std::string name_;
        std::unique_ptr<Acceptor> acceptor_;
        std::unique_ptr<EventLoopThreadPool> threadPool_;
        bool started_; // TODO 原子类型
        int nextConnectionId_; // always in loop thread
        ConnectionMap connectionMap_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        ThreadInitCallback threadInitCallback_;

        /**
         * 为新建的连接创建 Connection 对象
         * @param socket socket 文件描述符
         * @param peerAddress 客户端地址
         */
        void newConnection(Socket socket, const InternetAddress &peerAddress);

        /**
         * ---线程安全---
         * 删除 Connection 对象
         * @param connection Connection 对象
         */
        void removeConnection(const TcpConnectionPtr &connection);

        /**
         * 在 IO 线程中删除 Connection 对象
         * @param connection Connection 对象
         */
        void removeConnectionInLoop(const TcpConnectionPtr &connection);
    };
}


#endif //TINYWS_TCPSERVER_H
