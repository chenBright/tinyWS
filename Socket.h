#ifndef TINYWS_SOCKET_H
#define TINYWS_SOCKET_H

#include "base/noncopyable.h"

struct tcp_info; // in

namespace tinyWS {
    class InternetAddress;


    class Socket : noncopyable {
    public:
        explicit Socket(int sockfd);
        ~Socket();
        // TODO 检查移动构造函数的正确性
        Socket(Socket&& socket) noexcept; // 移动构造函数
        Socket& operator=(Socket &&rhs) noexcept; // 移动赋值

        /**
         * 获取 socketfd_
         * @return socketfd_
         */
        int fd() const;

        /**
         * 将 socketfd_ 设置为无效值（-1）
         */
        void setNoneFd();

        /**
         * socketfd_ 是否为有效文件描述符
         * @return true / false
         */
        bool isValid() const;

        void bindAddress(const InternetAddress &localAddress);
        void listen();
        int accept(InternetAddress *peerAddress);

        /**
         * shutdown write 端
         */
        void shutdownWrite();

        /**
         * 设置无阻塞
         * @param on
         */
        void setTcpNoDelay(bool on);
        void setReuseAddr(bool on);
        void setKeepAlive(bool on);

        int getSocketError();

    private:
        int sockfd_;
    };
}


#endif //TINYWS_SOCKET_H
