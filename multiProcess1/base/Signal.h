#ifndef TINYWS_SIGNAL_H
#define TINYWS_SIGNAL_H

#include <csignal>

#include <functional>
#include <string>
#include <iostream>

namespace tinyWS_process1 {

    class Signal {
    public:
        typedef void (*SignalCallback)(int);

        friend class SignalManager;

    private:
        int signo_;
        std::string name_;
        std::string meaning_;
        SignalCallback signalCallback_;

    public:
        Signal(int signo,
               const std::string& name,
               const std::string& meaing,
               const SignalCallback& cb)
               : signo_(signo),
                 name_(name),
                 meaning_(meaing),
                 signalCallback_(cb) {

//            std::cout << "class Signal constructor" << std::endl;
        }

        ~Signal() {
//            std::cout << "class Signal destructor" << std::endl;
        }

        int signo() const {
            return signo_;
        }

        std::string name() const {
            return name_;
        }

        std::string meaning() const {
            return meaning_;
        }

        bool isSame(int signal) const {
            return signo_ == signal;
        }
    };

    class SignalManager {
    private:
        unsigned int num_;

    public:
        SignalManager() : num_(0) {

        }

        ~SignalManager() {
//            std::cout << "class SignalManager destructor" << std::endl;
        }

        void addSignal(const Signal& sign) {
            updateSignal(sign.signo_, sign.signalCallback_);
            ++num_;
        }

        void updateSignal(const Signal& sign) {
            updateSignal(sign.signo_, sign.signalCallback_);
        }

        void deleteSignal(const Signal& sign) {
            removeSignal(sign.signo_);
            --num_;
        }

    private:
        void updateSignal(int signo, sighandler_t handler) {
            struct sigaction sa{};
            sa.sa_handler = handler;
            sa.sa_flags |= SA_RESTART;
            sigfillset(&sa.sa_mask);
            int result = sigaction(signo, &sa, nullptr);
            if (result == -1) {
                std::cerr << "update signal error: " << errno << std::endl;
            }
        }

        void removeSignal(int signo) {
            struct sigaction sa{};
            sa.sa_handler = nullptr;
            sa.sa_flags |= SA_RESTART;
            sigfillset(&sa.sa_mask);
            int result = sigaction(signo, &sa, nullptr);
            if (result == -1) {
                std::cerr << "remove signal error: " << errno << std::endl;
            }
        }
    };

}

#endif //TINYWS_SIGNAL_H
