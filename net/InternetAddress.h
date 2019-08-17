#ifndef TINYWS_INTERNETADDRESS_H
#define TINYWS_INTERNETADDRESS_H

#include <cstdint>
#include <netinet/in.h>

#include <string>

namespace tinyWS {
    class InternetAddress {
    public:
        /**
         * 构造函数
         * @param port 端口号
         */
        explicit InternetAddress(uint16_t port);

        /**
         * 构造函数
         * @param ip
         * @param port 端口号
         */
        InternetAddress(const std::string &ip, uint16_t port);

        /**
         * 构造函数
         * @param address  sockaddr_in 格式的地址结构体
         */
        explicit InternetAddress(const sockaddr_in &address);

        /**
         * 获取 IP
         * @return IP 字符串
         */
        std::string toIP() const;

        /**
         * 获取"IP:port"格式的字符串
         * @return "IP:port"格式的字符串
         */
        std::string toIpPort() const;

        /**
         * 获取 sockaddr_in 类型的地址信息
         * @return 地址信息
         */
        const sockaddr_in& getSockAddrInternet() const;

        /**
         * 设置地址信息
         * @param address sockaddr_in 类型的地址信息
         */
        void setSockAddrInternet(const sockaddr_in &address);

        /**
         * 获取 IP
         * @return IP
         */
        uint32_t ipNetEnd() const;

        /**
         * 获取端口号
         * @return 端口号
         */
        uint16_t portNetEnd() const;

        /**
         * 获取本地地址信息
         * @param sockfd socket fd
         * @return sockaddr_in 类型的地址信息
         */
        static sockaddr_in getLocalAddress(int sockfd);

        /**
         * 过去客户端地址信息
         * @param sockfd socket fd
         * @return sockaddr_in 类型的地址信息
         */
        static sockaddr_in getPeerAddress(int sockfd);

    private:
        sockaddr_in address_;
    };
}


#endif //TINYWS_INTERNETADDRESS_H
