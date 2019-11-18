//#include "ProcessPool.h"
//
//#include <cassert>
//#include <cstdio>
//#include <algorithm>
//
//#include "Process.h"
//#include "utility.h"
//
//using namespace std::placeholders;
//using namespace tinyWS_process;
//
//ProcessPool::ProcessPool(const std::string &name)
//    : name_(name),
//      running_(false) {
//
//}
//
//ProcessPool::~ProcessPool() {
//    if (running_) {
//        stop();
//    }
//}
//
//void ProcessPool::start(int numProcesses) {
//    assert(processes_.empty());
//
//    running_ = true;
//    processes_.reserve(static_cast<ProcessList::size_type>(numProcesses));
//    for (int i = 0; i < numProcesses; ++i) {
//        char id[32];
//        snprintf(id, sizeof(id), "%d", i);
////        processes_.push_back(tinyWS::make_unique<Process>(std::bind()))
//        processes_[i]->start();
//    }
//}
//
//void ProcessPool::stop() {
//    std::for_each(processes_.begin(), processes_.end(), std::bind(&Process::wait, _1));
//}
//
//void ProcessPool::run(const tinyWS_thread::ProcessPool::Task &task) {
//    dequeue_.push_back(task);
//}
//
//void ProcessPool::runInProcess(pid_t pid, int pipefd[2]) {
//    try {
//        while (running_) {
//
//        }
//    }
//
//}
