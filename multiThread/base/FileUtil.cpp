#include "FileUtil.h"

#include <cstdio>
#include <unistd.h>

using namespace tinyWS_thread;

FileUtil::FileUtil(std::string filename)
    : fp_(fopen(filename.c_str(), "ae")) {
    setbuffer(fp_, buffer_, sizeof(buffer_));
}

FileUtil::~FileUtil() {
    fclose(fp_);
}

void FileUtil::append(const char* logline, const size_t len) {
    size_t n = this->write(logline, len);
    size_t remain = len - n;
    while (remain > 0) {
        size_t tmp = this->write(logline + n, remain);
        if (tmp == 0) {
            int err = ::ferror(fp_);
            if (err) {
                fprintf(stderr, "FileUtil::append() failed !\n");
            }
            break;
        }
        remain -= tmp;
    }
}

void FileUtil::flush() {
    ::fflush(fp_);
}

size_t FileUtil::write(const char* logline, size_t len) {
    return ::fwrite_unlocked(logline, 1, len, fp_);
}
