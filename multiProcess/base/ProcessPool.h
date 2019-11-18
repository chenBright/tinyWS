//#ifndef TINYWS_PROCESSPOOL_H
//#define TINYWS_PROCESSPOOL_H
//
//#include <utility>
//#include <functional>
//#include <memory>
//#include <vector>
//#include <deque>
//#include <string>
//
//namespace tinyWS_process {
//    class Process;
//
//    class ProcessPool {
//    public:
//        using parentTask = std::function<void(int)>;
//        using childTask = std::function<void()>;
//        using Task = std::pair<parentTask, childTask>;
//
//    private:
//        using ProcessList = std::vector<std::unique_ptr<Process>>;
//        std::string name_;
//        ProcessList processes_;
//        std::deque<Task> dequeue_;
//        bool running_;
//
//    public:
//        explicit ProcessPool(const std::string& name = std::string());
//
//        ~ProcessPool();
//
//        void start(int numProcesses);
//
//        void stop();
//
//        void run(const Task& task);
//
//    private:
//        void runInProcess(pid_t pid, int pipefd[2]);
//
//        Task take();
//    };
//}
//
//
//#endif //TINYWS_PROCESSPOOL_H
