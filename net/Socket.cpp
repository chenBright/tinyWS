#include "Socket.h"

#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h> // struct tcp_info
#include <sys/socket.h>
#include <cerrno>

#include <iostream>

#include "../base/Logger.h"
#include "InternetAddress.h"

using namespace tinyWS;

Socket::Socket(int sockfd) : sockfd_(sockfd) {}

Socket::~Socket() {
    // 只有当 socketfd_ 是有效的描述符时，才关闭 socketfd_。
    if (isValid()) {
        debug(LogLevel::ERROR) << "Socket::~Socket() fd = " << sockfd_ << std::endl;
        ::close(sockfd_);
    }
}

Socket::Socket(Socket &&socket) noexcept : sockfd_(socket.fd()) {
    socket.setNoneFd();
}

Socket& Socket::operator=(Socket &&rhs) noexcept {
    if (this != &rhs) {
        sockfd_ = rhs.fd();
        rhs.setNoneFd();
    }
    return *this;
}

int Socket::fd() const {
    return sockfd_;
}

void Socket::setNoneFd() {
    sockfd_ = -1;
}

inline bool Socket::isValid() const {
    return sockfd_ >= 0;
}

void Socket::bindAddress(const InternetAddress &localAddress) {
    sockaddr_in address = localAddress.getSockAddrInternet();
    // TODO 学习 reinterpret_cast 和 static_cast 的区别
    int result = bind(sockfd_, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (result < 0) {
        debug(LogLevel::ERROR) << "Socket::bindAddress" << std::endl;
    }
}

void Socket::listen() {
    if (::listen(sockfd_, 128) < 0) {
        debug(LogLevel::ERROR) << "Socket::listen" << std::endl;
    }
}

int Socket::accept(InternetAddress *peerAddress) {
    sockaddr_in address{};
    auto addressLen = static_cast<socklen_t>(sizeof(address));
    // TODO 学习 Linux 新增的系统调用，直接 accept4() 得到非阻塞的 socket
    int connectionFd = ::accept4(sockfd_, reinterpret_cast<sockaddr*>(&address), &addressLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connectionFd >= 0) {
        peerAddress->setSockAddrInternet(address);
        return connectionFd;
    } else {
        int savedErrno = errno;
        debug(LogLevel::ERROR) << "Socket::accept" << std::endl;
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                debug(LogLevel::ERROR) << "unexpected error of ::accept "
                                             << savedErrno << std::endl;
                break;
            default:
                debug(LogLevel::ERROR) << "unknown error of ::accept "
                                             << savedErrno << std::endl;
                break;
        }
    }
}

void Socket::shutdownWrite() {
    if (shutdown(sockfd_, SHUT_RD) < 0) {
        debug(LogLevel::ERROR) << "Socket::shutdownWrite" << std::endl;
    }
}

void Socket::setTcpNoDelay(bool on) {
    int opt = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

void Socket::setReuseAddr(bool on) {
    int opt = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void Socket::setKeepAlive(bool on) {
    int opt = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
}

int Socket::getSocketError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    } else {
        return optval;
    }
}
