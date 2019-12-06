#ifndef TINYWS_TCPSERVER_H
#define TINYWS_TCPSERVER_H

#include <functional>
#include <memory>
#include <string>
#include <map>

#include "../base/noncopyable.h"
#include "TcpConnection.h"
#include "Socket.h"
#include "Timer.h"
#include "type.h"

namespace tinyWS_process2 {

    class EventLoop;
    class Acceptor;
    class ProcessPool;
    class InternetAddress;
    class TimerId;

    // TcpServer 的功能：管理 Acceptor 获得的 TcpConnection。
    // TcpServer 是供用户直接使用的，生命周期由用户控制。
    // 用户只需要设置好 callback，再调用 createProccesses() 即可。
    //
    // 尽量让依赖是单向的。
    // TcpServer 用到 Acceptor，但 Acceptor 并不知道 TcpServer 的存在。
    // TcpServer 创建 TcpConnection，但 TcpConnection 并不知道 TcpServer 的存在
    class TcpServer {
    public:
        using ProcessInitCallback = std::function<void(EventLoop*)>;

    private:
        using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

        EventLoop* loop_;
        const std::string name_;

        Socket socketBeforeFork_; // 用于在进程池 fork 子进程之前，保存 listen sockfd
        std::unique_ptr<Acceptor> acceptor_;
        std::unique_ptr<ProcessPool> processPool_;

        int nextConnectionId_;
        bool started_;
        ConnectionMap connectionMap_;


        ConnectionCallback connectionCallback_;             // 连接建立的回调函数
        MessageCallback messageCallback_;                   // 消息到来的回调函数
        ProcessInitCallback threadInitCallback_;             // 线程初始化的回调函数

    public:
        TcpServer(const InternetAddress &address, const std::string &name);

        ~TcpServer();

        EventLoop* getLoop() const;

        void start();

        /**
         * 设置连接建立的回调函数
         * @param cb 回调函数
         */
        void setConnectionCallback(const ConnectionCallback &cb);

        /**
         * 设置消息到来的回调函数
         * @param cb 回调函数
         */
        void setMessageCallback(const MessageCallback &cb);

        TimerId runAt(TimeType runTime, const Timer::TimerCallback& cb);

        TimerId runAfter(TimeType delay, const Timer::TimerCallback& cb);

        TimerId runEvery(TimeType interval, const Timer::TimerCallback& cb);

    private:
        void newConnectionInParent(Socket socket, const InternetAddress& peerAddress);

        void newConnectionInChild(EventLoop* loop, Socket socket);

        void removeConnection(const TcpConnectionPtr& connection);

        void clearInSubProcess(bool isParent);
    };
}


#endif //TINYWS_TCPSERVER_H
