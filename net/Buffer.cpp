#include "Buffer.h"

#include <cassert>
#include <endian.h>     // htobe32 htobe16 be32toh be16toh
#include <cstring>      // memcpy
#include <sys/uio.h>    // iovec

#include <algorithm>

using namespace tinyWS;

const char Buffer::kCRLF[] = "\r\n";

Buffer::Buffer()
    : buffer_(kCheapPrepend + kInitialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend) {
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
}

void Buffer::swap(tinyWS::Buffer &rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}

size_t Buffer::readableBytes() const {
    return writerIndex_ - readerIndex_;
}

size_t Buffer::writableBytes() const {
    return buffer_.size() - writerIndex_;
}

size_t Buffer::prependableBytes() const {
    return readerIndex_;
}

const char* Buffer::peek() const {
    return begin() + readerIndex_;
}

char* Buffer::beginWrite() {
    return begin() + writerIndex_;
}

const char* Buffer::beginWrite() const {
    return begin() + writerIndex_;
}

const char* Buffer::findCRLF() const {
    // 可使用 memmem() 代替 std::search()
    const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

const char* Buffer::findCRLF(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());

    const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

const char* Buffer::findEOL() const {
    const void *eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char*>(eol);
}

const char* Buffer::findEOL(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());

    const void *eol = memchr(start, '\n', beginWrite() - start);
    return static_cast<const char*>(eol);
}

void Buffer::retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
        readerIndex_ += len;
    } else {
        retrieveAll();
    }
}

void Buffer::retrieveUntil(const char *end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}

void Buffer::retrieveInt32() {
    retrieve(sizeof(int32_t));
}

void Buffer::retrieveInt16() {
    retrieve(sizeof(int16_t));
}

void Buffer::retrieveInt8() {
    retrieve(sizeof(int8_t));
}

void Buffer::retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes());
}

std::string Buffer::retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string str(peek(), len);
    retrieve(len);

    return str;
}

std::string Buffer::toString() const {
    std::string str(peek(), readableBytes());
    return str;
}

void Buffer::append(const std::string &str) {
    append(str.data(), str.size());
}

void Buffer::append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const void *data, size_t len) {
    append(static_cast<const char*>(data), len);
}

void Buffer::appendInt32(int32_t x) {
    int32_t be32 = htobe32(static_cast<uint32_t>(x)); // 将主机字节序转换为 32 位的大端字节序（网络字节序）
    append(&be32, sizeof(be32));
}

void Buffer::appendInt16(int16_t x) {
    int16_t be16 = htobe16(static_cast<uint16_t>(x)); // 将主机字节序转换为 16 位的大端字节序（网络字节序）
    append(&be16, sizeof(be16));
}

void Buffer::appendInt8(int8_t x) {
    append(&x, sizeof(x));
}

void Buffer::ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}

int32_t Buffer::readInt32() {
    int32_t result = peekInt32();
    retrieveInt32();

    return result;
}

int16_t Buffer::readInt16() {
    int16_t result = peekInt16();
    retrieveInt16();

    return result;
}

int8_t Buffer::readInt8() {
    int8_t result = peekInt8();
    retrieveInt8();

    return result;
}

int32_t Buffer::peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof(be32));

    return be32toh(static_cast<uint32_t>(be32));
}

int16_t Buffer::peekInt16() const {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof(be16));

    return be16toh(static_cast<uint16_t>(be16));
}

int8_t Buffer::peekInt8() const {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();

    return x;
}

void Buffer::prependInt32(int32_t x) {
    int32_t be32 = htobe32(static_cast<uint32_t>(x));
    prepend(&be32, sizeof(be32));
}

void Buffer::prependInt16(int16_t x) {
    int16_t be16 = htobe16(static_cast<uint16_t>(x));
    prepend(&be16, sizeof(be16));
}

void Buffer::prependInt8(int8_t x) {
    prepend(&x, sizeof(x));
}

void Buffer::prepend(const void *data, size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char *d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readerIndex_);
}

void Buffer::shrink(size_t reserve) {
    Buffer temp;
    temp.ensureWritableBytes(readableBytes() + reserve);
    swap(temp);
}

void Buffer::hasWritten(size_t len) {
    writerIndex_ += len;
}

ssize_t Buffer::readFd(int fd, int *savedErrno) {
    // 在 stack 上创建一个临时的缓冲区 extrabuf，这样使得缓冲区足够大，
    // 所以通常一次 readv(2) 调用就能读出全部数据。
    // 这样做的好处：
    // 1 不需要使用 ioctl(socketfd, FIONREAD< &length) 系统调用来
    //    获取有多少数据可读而提前预留 Buffer 的 capacity()。
    //    可以在一次读取之后将 extrabuf 中的数据 append() 到 Buffer 中。
    // 2 Buffer::readfd() 值调用了一次 read(2)，而不是反复调用 read(2) 直到其但会 EAGIN。因为：
    //      2.1 Epoll 采用的是 level trigger，这样做不会丢失数据或者消息；
    //      2.2 对于追求低延迟的程序来说，这么做是高效的，因为每次读数据只需要一次系统调用；
    //      2.3 这样做照顾了多个连接的公平性，不会因为某个连接上数据过大而影响其他连接处理数据。
    char extrabuf[65536];
    iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    ssize_t n = ::readv(fd, vec, 2);
    if (n < 0) {
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } else {
        // 使用了额外的缓冲区，将数据 append 到 buffer_ 上
        writerIndex_ = buffer_.size(); // 此时 buffer_ 已满，更新 writerIndex_ 到缓冲区末尾。
        append(extrabuf, n - writable);

        // 如果 n == writable + sizeof(extrabuf)，可能还有数据没读完，就再读一次。
        if (n == writable + sizeof(extrabuf)) {
            n += readFd(fd, savedErrno);
        }
    }

    return n;
}

char* Buffer::begin() {
    return &*buffer_.begin();
}

const char* Buffer::begin() const {
    return &*buffer_.begin();
}

void Buffer::makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        buffer_.resize(writerIndex_ + len);
    } else {
        // move readable data to the front, make space inside buffer
        assert(kCheapPrepend < readerIndex_); // 读过数据后，kCheapPrepend < readerIndex_，这才有移动的意义
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_  + readable;
        assert(readable == readableBytes());
    }
}