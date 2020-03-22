#include "AsyncLogger.h"

#include <pthread.h>
#include <cstdio>
//#include <ctime>
#include <sys/time.h>

#include "AsyncLogging.h"

using namespace tinyWS_thread;

static pthread_once_t ponce = PTHREAD_ONCE_INIT;
static AsyncLogging *asyncLoggingPtr;

void once_init() {
    asyncLoggingPtr = new AsyncLogging(AsyncLogger::getLogFilename());
    asyncLoggingPtr->start();
}

void output(const char* message, int len) {
    pthread_once(&ponce, once_init);
    asyncLoggingPtr->append(message, len);
}

AsyncLogger::Impl::Impl(const char* filename, int line)
    : stream_(),
      line_(line),
      basename_(filename) {
    formatTime();
}

void AsyncLogger::Impl::formatTime() {
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    ::gettimeofday(&tv, nullptr);
    time = tv.tv_sec;
    struct tm* p_time = ::localtime(&time);
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;
}

AsyncLogger::AsyncLogger(const char* filename, int line)
    : impl_(filename, line) {

}

AsyncLogger::~AsyncLogger() {
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
    const LogStream::Buffer& buffer(stream().buffer());
    output(buffer.data(), buffer.length());
}

LogStream& AsyncLogger::stream() {
    return impl_.stream_;
}

void AsyncLogger::setLogFilename(std::string filename) {
    logFilename_ = filename;
}

std::string AsyncLogger::getLogFilename() {
    return logFilename_;
}
