#ifndef TINYWS_INTERNETADDRESS_H
#define TINYWS_INTERNETADDRESS_H

#include <cstdint>
#include <netinet/in.h>

#include <string>

namespace tinyWS {
    class InternetAddress {
    public:
        explicit InternetAddress(uint16_t port);
        InternetAddress(const std::string &ip, uint16_t port);
        InternetAddress(const sockaddr_in &address);

        std::string toIP() const;
        std::string toIpPort() const;

        const sockaddr_in& getSockAddrInternet() const;
        void setSockAddrInternet(const sockaddr_in &address);

        uint32_t ipNetEnd() const;
        uint16_t portNetEnd() const;

        static sockaddr_in getLocalAddress(int sockfd);
        static sockaddr_in getPeerAddress(int sockfd);

    private:
        sockaddr_in address_;
    };
}


#endif //TINYWS_INTERNETADDRESS_H
