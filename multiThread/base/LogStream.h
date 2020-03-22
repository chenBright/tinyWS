#ifndef TINYWS_LOGSTREAM_H
#define TINYWS_LOGSTREAM_H

#include <cstring>

#include <string>

#include "noncopyable.h"

namespace tinyWS_thread {
    const int kSmallBuffer = 4000;
    const int kLargeBuffer = 4000 * 1000;

    template <int SIZE>
    class FixedBuffer : noncopyable {
    private:
        char data_[SIZE];
        char* cur_;

    public:
        FixedBuffer() : cur_(data_) {}

        ~FixedBuffer() = default;

        void append(const char* buffer, size_t len) {

        }

        const char* data() const {
            return data_;
        }

        int length() const {
            return static_cast<int>(cur_ - data_);
        }

        char* current() {
            return cur_;
        }

        int avail() const {
            return static_cast<int>(end() - cur_);
        }

        void add(size_t len) {
            cur_ += len;
        }

        void reset() {
            cur_ = data_;
        }

        void bzero() {
            ::memset(data_, 0, sizeof(data_));
        }

    private:
        const char* end() const {
            return data_ + sizeof(data_);
        }
    };

    class LogStream : noncopyable {
    public:
        using Buffer = FixedBuffer<kSmallBuffer>;

    private:
        Buffer buffer_;

        static const int kMaxNumericSize;

    public:
        LogStream& operator<<(bool flag);
        LogStream& operator<<(short);
        LogStream& operator<<(unsigned short);
        LogStream& operator<<(int);
        LogStream& operator<<(unsigned int);
        LogStream& operator<<(long);
        LogStream& operator<<(unsigned long);
        LogStream& operator<<(long long);
        LogStream& operator<<(unsigned long long);
        LogStream& operator<<(const void*);
        LogStream& operator<<(float n);
        LogStream& operator<<(double);
        LogStream& operator<<(long double);
        LogStream& operator<<(char c);
        LogStream& operator<<(char* str);
        LogStream& operator<<(const char* str);
        LogStream& operator<<(const unsigned char* str);
        LogStream& operator<<(const std::string& v);

        void append(const char* data, int len);

        const Buffer& buffer() const;

        void resetBuffer();

    private:
        void staticCheck();

        template <class T>
        void formatInteger(T data);
    };

    template <class T>
    inline LogStream& operator<<(LogStream& s, T data);
}

#endif //TINYWS_LOGSTREAM_H
