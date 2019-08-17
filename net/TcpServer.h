#ifndef TINYWS_TCPSERVER_H
#define TINYWS_TCPSERVER_H

#include <functional>
#include <memory>
#include <string>
#include <map>

#include "../base/noncopyable.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"
#include "CallBack.h"

namespace tinyWS {
    class EventLoop;
    class Acceptor;
    class InternetAddress;


    // TcpServer 的功能：管理 Acceptor 获得的 TcpConnection。
    // TcpServer 是供用户直接使用的，生命周期由用户控制。
    // 用户只需要设置好 callback，再调用 start() 即可。
    //
    // muduo 尽量让依赖是单向的。
    // TcpServer 用到 Acceptor，但 Acceptor 并不知道 TcpServer 的存在。
    // TcpServer 创建 TcpConnection，但 TcpConnection 并不知道 TcpServer 的存在。
    class TcpServer : noncopyable,
                      std::enable_shared_from_this<TcpServer> {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback; // 回调函数类型

        /**
         * 构造函数
         * @param loop 所属的 EventLoop
         * @param address server 的地址对象
         * @param name server 的名字
         */
        TcpServer(tinyWS::EventLoop *loop,
                  const tinyWS::InternetAddress &address,
                  const std::string &name);

        ~TcpServer();

        /**
         * 获取所属 EventLoop 的指针
         * @return 所属 EventLoop 的指针
         */
        EventLoop* getLoop() const;

        /**
         * 设置线程数来处理连接
         * - 当 threadNumber 为 0，表示 TcpServer 是单线程的，不会创建新线程，所有 IO 操作都在主线程完成；
         * - 当 threadNumber 大于 0，表示 TcpServer 是多线程的，会创建 event loop pool。
         *   多线程 TcpServer 所属的 EventLoop 只用来接受新连接，而新连接会使用 event loop pool 来执行 IO 操作。
         * @param threadNumber 线程数，必须大于等于 0。
         */
        void setThreadNumber(int threadNumber);

        /**
         * --- 安全线程 ---
         * 如果 Acceptor 为监听 socket，则调用该函数，启动服务，监听 socket。
         * 即使多次调用，也只启动一次。
         */
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

        /**
         * 设置线程初始化的回调函数
         * @param cb 回调函数
         */
        void setThreadInitCallback(const ThreadInitCallback &cb);


    private:
        // <连接名，TcpConnection 对象的智能指针> 类型
        typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

        EventLoop *loop_;                                   // Accept EventLoop
        const std::string name_;                            // TcpServer 名字，方便打印日志
        std::unique_ptr<Acceptor> acceptor_;                // Acceptor
        std::unique_ptr<EventLoopThreadPool> threadPool_;   // EventLoop 线程池
        bool started_;                                      // 是否启动 TODO 原子类型
        int nextConnectionId_;                              // 下一连接 ID，只会在 IO 线程中操作该值
        ConnectionMap connectionMap_;                       // <连接名，TcpConnection 对象的智能指针>

        ConnectionCallback connectionCallback_;             // 连接建立的回调函数
        MessageCallback messageCallback_;                   // 消息到来的回调函数
        ThreadInitCallback threadInitCallback_;             // 线程初始化的回调函数

        /**
         * 为新建立的连接创建 TcpConnectionPtr 对象
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
