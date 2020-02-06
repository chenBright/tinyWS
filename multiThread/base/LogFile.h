#ifndef TINYWS_LOGFILE_H
#define TINYWS_LOGFILE_H

#include <string>
#include <memory>

#include "noncopyable.h"
#include "MutexLock.h"
#include "FileUtil.h"

namespace tinyWS_thread {

    class LogFile : noncopyable {
    private:
        const std::string basename_;
        const int flushEveryN_;

        int count_;
        std::unique_ptr<MutexLock> mutex_;
        std::unique_ptr<FileUtil> file_;

    public:
        LogFile(const std::string& basename, int flushEveryN = 1024);

        ~LogFile() = default;

        void append(const char* logline, int len);

        void flush();

        void rollFile();

    private:
        void append_unlocked(const char* logline, int len);
    };
}

#endif //TINYWS_LOGFILE_H
