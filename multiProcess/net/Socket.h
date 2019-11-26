#ifndef TINYWS_SOCKET_H
#define TINYWS_SOCKET_H



#include "../base/noncopyable.h"

namespace tinyWS_process {

    class InternetAddress;

    class Socket : noncopyable {
    private:
        int sockfd_;

    public:
        explicit Socket(int sockfd);

        ~Socket();

        Socket(Socket&& socket) noexcept;

        Socket& operator=(Socket&& rhs) noexcept;

        int fd() const;

        void setNoneFd();

        bool isValid() const;

        void bindAddress(const InternetAddress& localAddress);

        void listen();

        int accept(InternetAddress* peerAddress);

        void shutdownWrite();

        void setTcpNoDelay(bool on);

        void setReuseAddr(bool on);

        void setKeepAlive(bool on);

        int getSocketError();
    };

}

#endif //TINYWS_SOCKET_H
