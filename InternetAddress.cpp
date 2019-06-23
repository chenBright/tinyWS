#include "InternetAddress.h"

#include <cstring>
#include <strings.h> // bzero()
#include <arpa/inet.h>

#include <iostream>

using namespace tinyWS;

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

InternetAddress::InternetAddress(uint16_t port) {
    bzero(&address_, sizeof(address_));
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htobe16(port);
}

InternetAddress::InternetAddress(const std::string &ip, uint16_t port) {
    bzero(&address_, sizeof(address_));
    address_.sin_family = AF_INET;
    address_.sin_port = htobe16(port);
    if (inet_pton(AF_INET, ip.c_str(), &address_.sin_addr) <= 0) {
        std::cout << "InternetAddress::InternetAddress(const std::string &ip, uint16_t port)" << std::endl;
    }
}

InternetAddress::InternetAddress(const sockaddr_in &address) : address_(address) {
}

std::string InternetAddress::toIP() const {
    const int size = 32;
    char buf[size];
    inet_ntop(AF_INET, &address_.sin_addr, buf, static_cast<socklen_t>(size));
    return buf;
}

std::string InternetAddress::toIpPort() const {
    const int size = 32;
    char buf[size];

    char host[INET_ADDRSTRLEN] = "INVALID";
    inet_ntop(AF_INET, &address_.sin_addr, host, static_cast<socklen_t>(strlen(host)));

    uint16_t port = be16toh(address_.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
    return buf;
}

const sockaddr_in& InternetAddress::getSockAddrInternet() const {
    return address_;
}

void InternetAddress::setSockAddrInternet(const sockaddr_in &address) {
    address_ = address;
}

uint32_t InternetAddress::ipNetEnd() const {
    return address_.sin_addr.s_addr;
}

uint16_t InternetAddress::portNetEnd() const {
    return address_.sin_port;
}

sockaddr_in InternetAddress::getLocalAddress(int sockfd) {
    sockaddr_in localAddress{};
    bzero(&localAddress, sizeof(localAddress));
    socklen_t addressLen = sizeof(localAddress);
    if (getsockname(sockfd, reinterpret_cast<sockaddr*>(&localAddress), &addressLen) < 0) {
        std::cout << "InternetAddress::getLocalAddress" << std::endl;
    }

    return localAddress;
}

sockaddr_in InternetAddress::getPeerAddress(int sockfd) {
    sockaddr_in peerAddress{};
    bzero(&peerAddress, sizeof(peerAddress));
    socklen_t addressLen = sizeof(peerAddress);
    if (getpeername(sockfd, reinterpret_cast<sockaddr*>(&peerAddress), &addressLen) < 0) {
        std::cout << "InternetAddress::getPeerAddress" << std::endl;
    }

    return peerAddress;
}