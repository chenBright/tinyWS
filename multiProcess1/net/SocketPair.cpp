#include "SocketPair.h"

#include <sys/socket.h>
#include <unistd.h> // close、getpid
#include <sys/types.h>
#include <sys/uio.h> // iovec
#include <cstdlib> // malloc、exit
#include <cassert>

#include <iostream>
#include <algorithm>

#include "EventLoop.h"
#include "TcpConnection.h"
#include "Channel.h"
#include "Socket.h"
#include "type.h"

using namespace tinyWS_process1;

SocketPair::SocketPair(EventLoop* loop, int fds[2])
    : loop_(loop),
      isParent_(true) {

    fds_[0] = fds[0];
    fds_[1] = fds[1];

//    std::cout << "class SocketPair constructor" << std::endl;
}

SocketPair::~SocketPair() {
    if (nullptr != pipeChannel_) {
        return;
    }

//    std::cout << "class SocketPair destructor" << std::endl;

    pipeChannel_->disableAll();
    pipeChannel_->remove();
    close(pipeChannel_->fd());
}
//
//SocketPair::SocketPair(SocketPair&& other) noexcept {
//    if (this == &other) {
//        return;
//    }
//
//    loop_ = other.loop_;
//    fds_[0] = fds_[0];
//    fds_[1] = fds_[1];
//    connection_ = std::move(other.connection_);
//    pipeChannel_ = std::move(other.pipeChannel_);
//    isParent_ = other.isParent_;
//    receiveFdCallback_ = std::move(other.receiveFdCallback_);
//    closeCallback_ = std::move(other.closeCallback_);
//
//    other.fds_[0] = -1;
//    other.fds_[1] = -1;
//    pipeChannel_ = nullptr;
//}
//
//SocketPair& SocketPair::operator=(SocketPair&& other) noexcept {
//    if (this != &other) {
//        loop_ = other.loop_;
//        fds_[0] = fds_[0];
//        fds_[1] = fds_[1];
//        connection_ = std::move(other.connection_);
//        pipeChannel_ = std::move(other.pipeChannel_);
//        isParent_ = other.isParent_;
//        receiveFdCallback_ = std::move(other.receiveFdCallback_);
//        closeCallback_ = std::move(other.closeCallback_);
//
//        other.fds_[0] = -1;
//        other.fds_[1] = -1;
//        pipeChannel_ = nullptr;
//    }
//
//    return *this;
//}

void SocketPair::setParentSocket() {
    assert(fds_[0] != -1 || fds_[1] != -1);

    close(fds_[1]);
    isParent_ = true;
//    std::cout << "switch parent:" << getpid() << std::endl;
//
//    std::cout << "socketpair parent set connection" << std::endl;
    // TODO name
    pipeChannel_.reset(new Channel(loop_, fds_[0]));
    pipeChannel_->setReadCallback(std::bind(&SocketPair::receiveFd, this));
    pipeChannel_->enableReading();
}

void SocketPair::setChildSocket() {
    assert(fds_[0] != -1 || fds_[1] != -1);

    close(fds_[0]);
    isParent_ = true;
//    std::cout << "switch child:" << getpid() << std::endl;

//    std::cout << "socketpair child set connection" << std::endl;
    // TODO name
    pipeChannel_.reset(new Channel(loop_, fds_[1]));
    pipeChannel_->setReadCallback(std::bind(&SocketPair::handleRead, this));
    pipeChannel_->enableReading();

}

void SocketPair::sendFdToChild(Socket socket) {
    assert(nullptr != pipeChannel_ || isParent_);

//    std::cout << "[parent] send fd: " << socket.fd() << ", pid = " << getpid() << std::endl;

    sendFd(std::move(socket));
}

void SocketPair::sendFdToParent(Socket socket) {
    assert(nullptr != pipeChannel_ || !isParent_);

//    std::cout << "[child] send fd: " << socket.fd() << ", pid = " << getpid() << std::endl;

    sendFd(std::move(socket));
}

void SocketPair::setReceiveFdCallback(const ReceiveFdCallback& cb) {
    receiveFdCallback_ = cb;
}

void SocketPair::setCloseCallback(const CloseCallback& cb) {
    closeCallback_ = cb;
}

void SocketPair::sendFd(Socket socket) {
    assert(nullptr != pipeChannel_);

    // 进程间传递文件描述符
    // 参考：https://blog.csdn.net/win_lin/article/details/7760951

    iovec vec[1];
    char c = 0;
    vec[0].iov_base = &c;
    vec[0].iov_len = 1;

    int cmsgsize = CMSG_LEN(sizeof(int));
    auto cmptr = (cmsghdr*)malloc(cmsgsize);
    if (cmptr == nullptr) {
//        std::cout << "[send_fd] init cmptr error" << std::endl;
        exit(1);
    }

    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    cmptr->cmsg_len = cmsgsize;

    msghdr msg{};
    msg.msg_iov = vec;
    msg.msg_iovlen = 1;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_control = cmptr;
    msg.msg_controllen = cmsgsize;
    *reinterpret_cast<int*>(CMSG_DATA(cmptr)) = socket.fd();

    int result = sendmsg(pipeChannel_->fd(), &msg, 0);
    free(cmptr);
    if (result == -1) {
//        std::cout << "[send_fd] sendmsg error" << std::endl;
        exit(1);
    }
}

int SocketPair::receiveFd() {
    assert(nullptr != pipeChannel_);

    int cmsgsize = CMSG_LEN(sizeof(int));
    auto cmptr = (cmsghdr*)malloc(cmsgsize);

    char buf[32]; // the max buf in msg is 32
    iovec vec[1];
    vec[0].iov_base = buf;
    vec[0].iov_len = sizeof(buf);

    msghdr msg{};
    msg.msg_iov = vec;
    msg.msg_iovlen = 1;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_control = cmptr;
    msg.msg_controllen = cmsgsize;

    int result = recvmsg(pipeChannel_->fd(), &msg, 0);


    if (result == -1) {
//        std::cout << "[send_fd] recvmsg error" << std::endl;

        return -1;
    }

    int acceptFd = *(int*)CMSG_DATA(cmptr);
//    std::cout << "receive fd: " << acceptFd << ", pid = " << getpid() << std::endl;

//    free(cmptr);

    return acceptFd;
}

void SocketPair::handleRead() {
    int sockfd = receiveFd();
    if (sockfd < 0) {
//        std::cout << "SocketPair::handleRead error" << std::endl;
    } else if (sockfd == 0) {
        // TODO 关闭？
//        std::cout << "TcpConnection::handleRead, result = 0" << std::endl;

        if (closeCallback_) {
            closeCallback_();
        }
    } else {
        if (receiveFdCallback_) {
            receiveFdCallback_(sockfd);
        }
    }
}
