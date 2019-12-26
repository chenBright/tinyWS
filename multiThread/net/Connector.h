#ifndef TINYWS_CONNECTOR_H
#define TINYWS_CONNECTOR_H

#include <memory>
#include <functional>

#include "../base/noncopyable.h"
#include "InternetAddress.h"

namespace tinyWS_thread {

    class EventLoop;
    class Channel;

    class Connector : noncopyable,
                      public std::enable_shared_from_this<Connector> {
    public:
        using NewConnectionCallback = std::function<void(int sockfd)>;

        Connector(EventLoop* loop, const InternetAddress &serverAddress);

        ~Connector();

        void setNewConnectionCallback(const NewConnectionCallback& cb);


        void start();

        void restart();

        void stop();

        const InternetAddress& serverAddress() const;

    private:
        enum States { kDisconnected, kConnecting, kConnected };
        static const int kMaxRetryDelayMs = 30 * 1000;
        static const int kInitRetryDelayMs = 500;

        EventLoop* loop_;
        InternetAddress serverAddress_;
        bool connect_;
        States state_;
        std::unique_ptr<Channel> channel_;
        NewConnectionCallback newConnectionCallback_;
        int retryDelayMs_;

        void setState(States state);

        void startInLoop();

        void stopInLoop();

        void connect();

        void connecting(int sockfd);

        void handleWrite();

        void handleError();

        void retry(int sockfd);

        int removeAndResetChannel();

        void resetChannel();

        int getSocketError(int sockfd);

        bool isSelfConnect(int sockfd);
    };
}


#endif //TINYWS_CONNECTOR_H
