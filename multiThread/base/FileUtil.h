#ifndef TINYWS_FILEUTIL_H
#define TINYWS_FILEUTIL_H

#include <string>

#include "noncopyable.h"

namespace tinyWS_thread {

    class FileUtil : noncopyable {
    public:
        static const int kBufferSize = 65536;

    public:
        explicit FileUtil(std::string filename);

        ~FileUtil();

        void append(const char* logline, const size_t len);

        void flush();

    private:
        size_t write(const char* logline, size_t len);

        FILE* fp_;
        char buffer_[kBufferSize];
    };
}

#endif //TINYWS_FILEUTIL_H
