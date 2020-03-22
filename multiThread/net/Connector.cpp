#include "Connector.h"

#include <cassert>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "Channel.h"
#include "../base/Logger.h"
#include "TimerId.h"

using namespace tinyWS_thread;

__thread char t_errnobuf[512];

const int Connector::kMaxRetryDelayMs;
const int Connector::kInitRetryDelayMs;

Connector::Connector(EventLoop *loop, const InternetAddress &serverAddress)
    : loop_(loop),
      serverAddress_(serverAddress),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(Connector::kInitRetryDelayMs) {

//    debug() << "ctor[" << this << "]" << std::endl;
}

Connector::~Connector() {
//    debug() << "dtor[" << this << "]" << std::endl;
    assert(!channel_);
}

void Connector::setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = Connector::kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
    // FIXME: cancel timer
}

const InternetAddress& Connector::serverAddress() const {
    return serverAddress_;
}

void Connector::setState(States state) {
    state_ = state;
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_) {
        connect();
    } else {
//        debug() << "do not connect";
    }
}

void Connector::stopInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnecting) {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        debug(LogLevel::ERROR) << "sockets::createNonblockingOrDie" << std::endl;
    }

    sockaddr_in address = serverAddress_.getSockAddrInternet();
    int result = ::connect(sockfd,
                           reinterpret_cast<sockaddr*>(&address),
                           static_cast<socklen_t>(sizeof(struct sockaddr_in6)));

    int savedErrno = (result == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            debug(LogLevel::ERROR) << "connect error in Connector::startInLoop " << savedErrno;
            ::close(sockfd);
            break;

        default:
            debug(LogLevel::ERROR) << "Unexpected error in Connector::startInLoop " << savedErrno;
            ::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::connecting(int sockfd) {
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));

    // channel_->tie(shared_from_this()); is not working,
    // as channel_ is not managed by shared_ptr
    channel_->enableWriting();
}

void Connector::handleWrite() {
//    debug() << "Connector::handleWrite " << state_;

    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();

        int err = getSocketError(sockfd);

        if (err) {
            debug(LogLevel::ERROR) << "Connector::handleWrite - SO_ERROR = "
                                   << err << " "
                                   << ::strerror_r(err, t_errnobuf, sizeof(t_errnobuf)) << std::endl;
            retry(sockfd);
        } else if (isSelfConnect(sockfd)) {
//            debug() << "Connector::handleWrite - Self connect" << std::endl;
            retry(sockfd);
        } else {
            setState(kConnected);
            if (connect_) {
                newConnectionCallback_(sockfd);
            } else {
                ::close(sockfd);
            }
        }
    } else {
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError() {
    debug(LogLevel::ERROR) << "Connector::handleError" << std::endl;
    assert(state_ == kConnecting);

    int sockfd = removeAndResetChannel();
    int err = getSocketError(sockfd);
    debug(LogLevel::ERROR) << "SO_ERROR = " << err << " "
                                 << ::strerror_r(err, t_errnobuf, sizeof(t_errnobuf))
                                 << std::endl;
    retry(sockfd);
}

void Connector::retry(int sockfd) {
    ::close(sockfd);
    setState(kDisconnected);
    if (connect_) {
//        debug() << "Connector::retry - Retry connecting to " << serverAddress_.toIPPort()
//                << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_ / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, Connector::kMaxRetryDelayMs);
    } else {
//        debug() << "do not connect" << std::endl;
    }
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));

    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

int Connector::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    } else {
        return optval;
    }
}

bool Connector::isSelfConnect(int sockfd) {
    struct sockaddr_in localAddress = InternetAddress::getLocalAddress(sockfd);
    struct sockaddr_in peerAddress = InternetAddress::getPeerAddress(sockfd);

    return localAddress.sin_port == peerAddress.sin_port &&
           localAddress.sin_addr.s_addr == peerAddress.sin_addr.s_addr;
}
