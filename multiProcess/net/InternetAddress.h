#ifndef TINYWS_INTERNETADDRESS_H
#define TINYWS_INTERNETADDRESS_H

#include <cstdint>
#include <netinet/in.h>

#include <string>
namespace tinyWS_process {

    class InternetAddress {
    private:
        sockaddr_in address_;

    public:
        explicit InternetAddress(uint16_t port = 0);

        InternetAddress(const std::string& ip, uint16_t port);

        explicit InternetAddress(const sockaddr_in& address);

        std::string toIP() const;

        std::string toIPPort() const;

        const sockaddr_in& getSockAddrInternet() const;

        void setSockAddrInternet(const sockaddr_in &address);

        uint32_t ipNetEnd() const;

        uint16_t portNetEnd() const;

        static sockaddr_in getLocalAddress(int sockfd);

        static sockaddr_in getPeerAddress(int sockfd);
    };
}


#endif //TINYWS_INTERNETADDRESS_H
