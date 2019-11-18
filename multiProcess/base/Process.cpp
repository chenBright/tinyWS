//#include "Process.h"
//
//#include <cassert>
//#include <unistd.h>
//#include <sys/socket.h>
//#include <sys/wait.h>
//
//#include <iostream>
//
//using namespace tinyWS_process;
//
//Process::Process(
//        const tinyWS_thread::Process::ProcessFunction& parentFunc,
//        const tinyWS_thread::Process::ProcessFunction& childFunc,
//        const std::string& name)
//    : started_(false),
//      pid_(-1),
//      pipefd_{}, // 不需要括号，使用空 initializer_list 初始化。
//      parentFunc_(parentFunc),
//      childFunc_(childFunc),
//      name_(name) {
//    // 初始化 pipefd_
//    socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd_);
//}
//
//Process::~Process() {
//    if (pid_ == 0) {
//        close(pipefd_[0]);
//    } else {
//        close(pipefd_[1]);
//    }
//}
//
//void Process::start() {
//    assert(!started_);
//
//    started_ = true;
//
//    pid_ = fork();
//
//    if (pid_ == 0) {
//        close(pipefd_[1]);
//        run();
//    } else {
//        close(pipefd_[0]);
//    }
//}
//
//bool Process::started() const {
//    return started_;
//}
//
//int Process::wait() {
//    return waitpid(pid_, nullptr, 0);
//}
//
//void Process::run() {
//    try {
//        if (pid_ == 0) {
//            childFunc_(pipefd_[0]);
//        } else {
//            parentFunc_(pipefd_[1]);
//        }
//    } catch (const std::exception &ex) {
//        std::cout << "exception caught in Ptocess " << pid_ << std::endl;
//        std::cout << "reason: " << ex.what() << std::endl;
//    } catch (...) {
//        throw; // rethrow
//    }
//}