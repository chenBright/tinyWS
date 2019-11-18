//#ifndef TINYWS_PROCESS_H
//#define TINYWS_PROCESS_H
//
//#include <sched.h>
//
//#include <array>
//#include <string>
//#include <functional>
//
//#include "noncopyable.h"
//
//namespace tinyWS_process {
//    class Process : public noncopyable {
//    public:
//        using ProcessFunction = std::function<void(int)>;
//
//    private:
//        bool started_;
//        pid_t pid_;
//        int pipefd_[2];
//        ProcessFunction parentFunc_;
//        ProcessFunction childFunc_;
//        std::string name_;
//
//
//    public:
//        Process(const tinyWS_thread::Process::ProcessFunction &parentFunc,
//                const tinyWS_thread::Process::ProcessFunction &childFunc,
//                const std::string& name);
//
//        ~Process();
//
//        void start();
//
//        bool started() const;
//
//        int wait();
//
//        const std::string& name() const;
//
//    private:
//        void run();
//    };
//}
//
//
//#endif //TINYWS_PROCESS_H
