#ifndef TINYWS_TCPCLIENT_H
#define TINYWS_TCPCLIENT_H

#include <string>
#include <memory>

#include "../base/noncopyable.h"
#include "TcpConnection.h"
#include "InternetAddress.h"
#include "CallBack.h"
#include "../base/MutexLock.h"

namespace tinyWS_thread {

    class EventLoop;
    class Connector;

    class TcpClient : noncopyable {
    public:
        using ConnectorPtr = std::shared_ptr<Connector>;

        TcpClient(EventLoop* loop, const InternetAddress& serverAddress, const std::string& name);

        ~TcpClient();  // force out-line dtor, for scoped_ptr members.

        void connect();

        void disconnect();

        void stop();

        TcpConnectionPtr connection() const;

        bool retry() const;

        void enableRetry();

        void setConnectionCallback(const ConnectionCallback& cb);

        void setMessageCallback(const MessageCallback& cb);

        void setWriteCompleteCallback(const WriteCompleteCallback& cb);

    private:
        EventLoop* loop_;
        ConnectorPtr connector_;
        const std::string name_;
        ConnectionCallback connectionCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        MessageCallback messageCallback_;
        bool retry_;
        bool connect_;
        int nextConnectionId_;
        mutable MutexLock mutex_;
        TcpConnectionPtr connection_;


        /// Not thread safe, but in loop
        void newConnection(int sockfd);
        /// Not thread safe, but in loop
        void removeConnection(const TcpConnectionPtr& conntion);
    };

}

#endif //TINYWS_TCPCLIENT_H
