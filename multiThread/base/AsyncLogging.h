#ifndef TINYWS_ASYNCLOGGING_H
#define TINYWS_ASYNCLOGGING_H

#include <vector>
#include <string>
#include <memory>

#include "noncopyable.h"
#include "LogStream.h"
#include "Thread.h"
#include "MutexLock.h"
#include "Condition.h"
#include "CountDownLatch.h"

namespace tinyWS_thread {

    class AsyncLogging : noncopyable {
    private:
        using Buffer =  FixedBuffer<kLargeBuffer>;
        using BufferPtr = std::shared_ptr<Buffer>;
        using BufferVector =  std::vector<BufferPtr>;

        const int flushInterval_;
        bool running_;
        std::string basename_;
        Thread thread_;
        MutexLock mutex_;
        Condition condition_;
        BufferPtr currentBuffer_;
        BufferPtr nextBuffer_;
        BufferVector buffers_;
        CountDownLatch latch_;

    public:
        AsyncLogging(const std::string& basename, int flushInterval = 2);

        ~AsyncLogging();

        void append(const char* logline, int len);

        void start();

        void stop();

    private:
        void threadFunction();
    };
}

#endif //TINYWS_ASYNCLOGGING_H
