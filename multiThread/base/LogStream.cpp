#include "LogStream.h"

#include <cstdio>

#include <algorithm>

using namespace tinyWS_thread;

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;

template <class T>
size_t convert(char buffer[], T value) {
    T i = value;
    char* p = buffer;

    do {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buffer, p);

    return p - buffer;
}


template class tinyWS_thread::FixedBuffer<kSmallBuffer>;
template class tinyWS_thread::FixedBuffer<kLargeBuffer>;

const int LogStream::kMaxNumericSize = 32;

LogStream& LogStream::operator<<(bool flag) {
    buffer_.append(flag ? "1" : "0", 1);

    return *this;
}

LogStream& LogStream::operator<<(short n) {
    *this << static_cast<int>(n);

    return *this;
}

LogStream& LogStream::operator<<(unsigned short n) {
    *this << static_cast<unsigned int>(n);

    return *this;
}

LogStream& LogStream::operator<<(int n) {
    formatInteger(n);

    return *this;
}

LogStream& LogStream::operator<<(unsigned int n) {
    formatInteger(n);

    return *this;
}

LogStream& LogStream::operator<<(long n) {
    formatInteger(n);

    return *this;
}

LogStream& LogStream::operator<<(unsigned long n) {
    formatInteger(n);

    return *this;
}

LogStream& LogStream::operator<<(long long n) {
    formatInteger(n);

    return *this;
}

LogStream& LogStream::operator<<(unsigned long long n) {
    formatInteger(n);

    return *this;
}

LogStream& LogStream::operator<<(const void* ptr) {
    uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
    if (buffer_.avail() > kMaxNumericSize) {
        char* buffer = buffer_.current();
        buffer[0] = '0';
        buffer[1] = 'x';
        size_t len = convert(buffer + 2, p);
        buffer_.add(len + 2);
    }

    return *this;
}

LogStream& LogStream::operator<<(float n) {
    *this << static_cast<double>(n);

    return *this;
}

LogStream& LogStream::operator<<(double n) {
    if (buffer_.avail() > kMaxNumericSize) {
        int len = ::snprintf(buffer_.current(), kMaxNumericSize, "%.12g", n);
        buffer_.add(len);
    }

    return *this;
}

LogStream& LogStream::operator<<(long double n) {
    if (buffer_.avail() > kMaxNumericSize) {
        int len = ::snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", n);
        buffer_.add(len);
    }

    return *this;
}

LogStream& LogStream::operator<<(char c) {
    buffer_.append(&c, 1);
    return *this;
}

LogStream& LogStream::operator<<(const char* str) {
    if (str) {
        buffer_.append(str, strlen(str));
    } else {
        buffer_.append("(null)", 6);
    }

    return *this;
}

LogStream& LogStream::operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
}

LogStream& LogStream::operator<<(const std::string& v) {
    buffer_.append(v.c_str(), v.size());

    return *this;
}

void LogStream::append(const char* data, int len) {
    buffer_.append(data, len);
}

const LogStream::Buffer& LogStream::buffer() const {
    return buffer_;
}

void LogStream::resetBuffer() {
    buffer_.reset();
}

void LogStream::staticCheck() {
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                  "LogStream::staticCheck() error");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                  "LogStream::staticCheck() error");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                  "LogStream::staticCheck() error");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                  "LogStream::staticCheck() error");
}

template <class T>
void LogStream::formatInteger(T data) {
    if (buffer_.avail() >= kMaxNumericSize) {
        size_t len = convert(buffer_.current(), data);
        buffer_.add(len);
    }
}

template <class T>
inline LogStream& operator<<(LogStream& s, T data) {
    s << data;

    return s;
}
