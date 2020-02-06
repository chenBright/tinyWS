#include "AsyncLogging.h"

#include <cassert>
#include <cstdio>

#include <functional>

#include "LogFile.h"
#include "../net/Timer.h"

using namespace tinyWS_thread;

AsyncLogging::AsyncLogging(const std::string& basename, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      thread_(std::bind(&AsyncLogging::threadFunction, this), "Logging"),
      mutex_(),
      condition_(mutex_),
      currentBuffer_(std::make_shared<Buffer>()),
      nextBuffer_(std::make_shared<Buffer>()),
      buffers_(),
      latch_(1) {
    assert(basename.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

AsyncLogging::~AsyncLogging() {
    if (running_) {
        stop();
    }
}

void AsyncLogging::append(const char* logline, int len) {
    MutexLockGuard lock(mutex_);
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline, len);
    } else {
        buffers_.push_back(currentBuffer_);
        currentBuffer_->reset();
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        condition_.notify();
    }
}

void AsyncLogging::start() {
    running_ = true;
    thread_.start();
    latch_.wait();
}

void AsyncLogging::stop() {
    running_ = false;
    condition_.notify();
    thread_.join();
}

void AsyncLogging::threadFunction() {
    assert(running_ = false);
    latch_.countDown();
    LogFile output(basename_);
    BufferPtr newBuffer1(std::make_shared<Buffer>());
    BufferPtr newBuffer2(std::make_shared<Buffer>());
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        {
            MutexLockGuard lock(mutex_);
            if (buffers_.empty()) {
                condition_.waitForSecond(flushInterval_);
            }
            buffers_.push_back(currentBuffer_);
            currentBuffer_->reset();

            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_) {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(~buffersToWrite.empty());

        if (buffersToWrite.size() > 25) {
            char buffer[256];
            ::snprintf(buffer, sizeof(buffer),
                      "Dropped log messages at %s, %zd larger buffers\n",
                       std::to_string(Timer::now()).c_str(), buffersToWrite.size() - 2);
            fputs(buffer, stderr);
            output.append(buffer, static_cast<int>(strlen(buffer)));
            buffersToWrite.erase(buffersToWrite.begin());
        }

        for (const auto& buf : buffersToWrite) {
            output.append(buf->data(), buf->length());
        }

        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }

    output.flush();
}