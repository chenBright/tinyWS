#ifndef TINYWS_ASYNCLOGGER_H
#define TINYWS_ASYNCLOGGER_H

#include <string>

#include "LogStream.h"

namespace tinyWS_thread {

    class AsyncLogger {
    private:
        class Impl {
        public:
            LogStream stream_;
            int line_;
            std::string basename_;

        public:
            Impl(const char* filename, int line);

            void formatTime();
        };

        Impl impl_;
        static std::string logFilename_;

    public:
        AsyncLogger(const char* filename, int line);

        ~AsyncLogger();

        LogStream& stream();

        static void setLogFilename(std::string filename);

        static std::string getLogFilename();
    };

#define LOG AsyncLogger(__FILE__, __LINE__).stream()
}

#endif //TINYWS_ASYNCLOGGER_H
