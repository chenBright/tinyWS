#ifndef TINYWS_SOCKET_H
#define TINYWS_SOCKET_H

#include "../base/noncopyable.h"

struct tcp_info; // in

namespace tinyWS {
    class InternetAddress;


    class Socket : noncopyable {
    public:
        /**
         * 构造函数
         * @param sockfd socket fd
         */
        explicit Socket(int sockfd);

        ~Socket();

        // Socket 对象的移动策略：
        // 将当前对象的 socketfd_ 复制给移动后对象，
        // 并将自己的 socketfd_ 设置为 -1（即无效的文件描述符）。
        // 对象析构的时候，先检查 socketfd_ 是否有效，再关闭 socketfd_。
        Socket(Socket&& socket) noexcept;               // 移动构造函数

        Socket& operator=(Socket &&rhs) noexcept;       // 移动赋值

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

        /**
         * 绑定地址
         * @param localAddress 地址对象
         */
        void bindAddress(const InternetAddress &localAddress);

        /**
         * 监听
         */
        void listen();

        /**
         * 接受连接，并返回 socket
         * @param peerAddress
         * @return
         */
        int accept(InternetAddress *peerAddress);

        /**
         * shutdown write 端
         */
        void shutdownWrite();

        /**
         * 禁用 Nagle 算法
         * @param on
         */
        void setTcpNoDelay(bool on);

        /**
         * 设置端口复用
         * @param on
         */
        void setReuseAddr(bool on);

        /**
         * 设置 keep alive
         * @param on
         */
        void setKeepAlive(bool on);

        /**
         * 获取 socket 错误码
         * @return
         */
        int getSocketError();

    private:
        int sockfd_; // socket 文件描述符
    };
}


#endif //TINYWS_SOCKET_H
