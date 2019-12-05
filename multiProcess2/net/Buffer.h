#ifndef TINYWS_BUFFER_H
#define TINYWS_BUFFER_H

#include <vector>
#include <string>

#include "../base/noncopyable.h"

namespace tinyWS_process {
    class Buffer : public noncopyable {
    public:
        // 在缓冲区前方增加 perpendable 区域，以应对需要在数据前面添加信息的场景。
        // 这样就不需要往后移动数组，以挪出空间来放置信息。简化实现，以空间换时间。
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

    private:
        std::vector<char> buffer_;  // 缓冲区
        size_t readerIndex_;        // 可取区域的起始索引
        size_t writerIndex_;        // 可写区域的起始索引，其中 readerIndex_ <= writerIndex_ < buffer_.size()

        static const char kCRLF[];  // CRLF = "\r\n"，HTTP 以 CRLF 结尾

    public:
        Buffer();

        // 使用默认的拷贝构造函数、赋值函数和析构函数

        void swap(Buffer &rhs);

        /**
         * 可取数据的大小
         * @return  数据大小
         */
        size_t readableBytes() const;

        /**
         * 可写入数据的空间大小
         * @return 空间大小
         */
        size_t writableBytes() const;

        /**
         * 头部预留空间的大小
         * @return 空间大小
         */
        size_t prependableBytes() const;

        /**
         * 获取可读区域的起始地址
         * @return 指向起始地址的指针
         */
        const char* peek() const;

        /**
         * 获取可写区域的起始地址
         * @return 指向起始地址的指针
         */
        char* beginWrite();

        // 同上
        const char* beginWrite() const;

        /**
         * 查找第一个 CRLF
         * @return CRLF 的指针
         */
        const char* findCRLF() const;

        /**
         * 查找从 start 位置起的第一个 CRLF
         * @param start 始查找的位置
         * @return CRLF 的指针
         */
        const char* findCRLF(const char *start) const;

        /**
         * 查找第一个回车符
         * @return 回车符的指针
         */
        const char* findEOL() const;

        /**
         * 查找从 start 位置起的第一个回车符
         * @param start 开始查找的位置
         * @return 回车符的指针
         */
        const char* findEOL(const char* start) const;

        // 下面一系列接口用于更新缓冲区的索引信息

        /**
         * 缓冲区可读区容量减少 len
         * 当 len > readableBytes()，即大于可读区域大小是，将缓冲区重置为初始状态。
         * @param len 需要减少的容量的大小
         */
        void retrieve(size_t len);

        /**
         * 减少缓冲区容量，将可取区域的起始索引往移动到 end 指针所指的位置。
         * peek() <= end <= beginWrite()
         * @param end 容量减少后，可读区域的其实位置
         */
        void retrieveUntil(const char *end);

        // Read int32_t from network endian

        /**
         * 缓冲区可读区容量减少 sizeof(int32_t)
         */
        void retrieveInt32();

        /**
         * 缓冲区可读区容量减少 sizeof(int16_t)
         */
        void retrieveInt16();

        /**
         * 缓冲区可读区容量减少 sizeof(int8_t)
         */
        void retrieveInt8();

        /**
         * 重置缓冲区
         */
        void retrieveAll();


        // 上面一系列 retrieve* 函数返回 void，是为了防止一下用法：
        // string str(retrieve(readableBytes()), readableBytes());
        // 该用法的行为是未定义的。

        /**
         * 将可读区域容量减少到 0，并读出该部分的数据。
         * @return 可读区域数据
         */
        std::string retrieveAllAsString();

        /**
         * 缓冲区可读区容量减少 len，并读出该部分的数据。
         * @param len 需要减少的容量的大小
         * @return 减少区域的数据
         */
        std::string retrieveAsString(size_t len);

        /**
         * 读出可读区域的数据
         * @return 可读区域的数据
         */
        std::string toString() const;

        // 下面一系列接口用于添加数据到缓冲区，添加数据的方式都是在添加到可读区域后面。

        /**
         * C++ 风格
         * 添加字符串数据到可读区域后面。
         * @param str 数据
         */
        void append(const std::string &str);

        /**
         * C 风格
         * 添加 len 大小的数据到可读区域后面。
         * @param data 数据的起始地址
         * @param len 要添加的长度
         */
        void append(const char *data, size_t len);

        // 同上，但需要将 data 转换成 const char* 类型
        void append(const void *data, size_t len);

        /**
         * 添加 32 位的大端字节序到可读区域后面。
         * @param x 需要添加的值
         */
        void appendInt32(int32_t x);

        /**
         * 添加 16 位的大端字节序到可读区域后面。
         * @param x 需要添加的值
         */
        void appendInt16(int16_t x);

        /**
         * 添加 8 位的大端字节序到可读区域后面。
         * @param x 需要添加的值
         */
        void appendInt8(int8_t x);

        /**
         * 确保可写区域能容纳得下 len 大小的数据。如果容纳不小，则将 buffer_ 扩容或者调整。
         * @param len 需要写入的数据的大小
         */
        void ensureWritableBytes(size_t len);

        /**
         * 写入了数据，更新写指针
         * @param len 写入数据的长度
         */
        void hasWritten(size_t len);

        /**
         * 读取 32 位的主机字节序数据，并调整缓冲区的索引信息。
         * @return 32 位的主机字节序的数据
         */
        int32_t readInt32();

        /**
         * 读取 16 位的主机字节序数据，并调整缓冲区的索引信息。
         * @return 16 位的主机字节序数据
         */
        int16_t readInt16();

        /**
         * 读取 8 位的数据，并将大端字节序转换为主机字节序。
         * @return 8 位的主机字节序的数据
         */
        int8_t readInt8();

        /**
         * 读取 32 位的数据，将其从大端字节序转换为主机字节序，最后返回。
         * @return 32 位的主机字节序数据
         */
        int32_t peekInt32() const;

        /**
         * 读取 16 位的数据，将其从大端字节序转换为主机字节序，最后返回。
         * @return 16 位的主机字节序数据
         */
        int16_t peekInt16() const;

        /**
         * 读取 8 位的数据，将其从大端字节序转换为主机字节序，最后返回。
         * @return 8 位的主机字节序数据
         */
        int8_t peekInt8() const;

        // 下面一系列接口用于添加数据到缓冲区，添加数据的方式都是在添加到 perpendable 区域。

        /**
         * 添加 32 位的大端字节序到 perpendable 区域。
         * @param x 需要添加的值
         */
        void prependInt32(int32_t x);

        /**
         * 添加 16 位的大端字节序到 perpendable 区域。
         * @param x 需要添加的值
         */
        void prependInt16(int16_t x);

        /**
         * 添加 8 位的大端字节序到 perpendable 区域。
         * @param x 需要添加的值
         */
        void prependInt8(int8_t x);

        /**
         * 添加 len 大小到 perpendable 区域。
         * @param data 数据的起始地址
         * @param len 要添加的长度
         */
        void prepend(const void *data, size_t len);

        /**
         * 重置缓冲区，使可写区域的容量至少为 readableBytes() + reserve，可读区域的容量为 0。
         * 该函数会重新分配内存空间，所以调用该函数之前得到的指针都会失效。
         * 可写区域的容量至少为 readableBytes() + reserve 的原因：
         * 该函数的实现是创建一个临时缓冲区，并调用 ensureWritableBytes() 函数，
         * 来保证可写区域的容量至少为 readableBytes() + reserve。
         * 而新建缓冲区的时候，可写区域的容量的初始值为 kInitialSize。
         * 如果 readableBytes() + reserve 小于 kInitialSize，则实际的可写区域的容量为 kInitialSize。
         * 如果 readableBytes() + reserve 大于 kInitialSize，则实际的可写区域的容量会调整为 readableBytes() + reserve。
         * @param reserve
         */
        void shrink(size_t reserve);

        /**
         * 从 fd 读取数据到缓冲区。
         * @param fd 文件描述符，通常情况下是 socket fd
         * @param savedErrno 错误信息
         * @return 读取到的数据的大小
         */
        ssize_t readFd(int fd);



    private:

        /**
         * 获取整个缓冲区的首地址
         * @return 首地址
         */
        char* begin();

        // 同上
        const char* begin() const;

        /**
         * 调整缓冲区的结构或者扩容
         * 如果前面 prependable 区域 + writable 区域 小于 len + kCheapPrepend，则将 buffer_ 扩容；
         * 否则，缓冲区的"空闲"空间可以容纳的下 len + kCheapPrepend 大小的数据，直接将可读区域的数据向前移动，
         * 相当于将前面的"空闲"空间移动到后面，增大可写区域，以容纳 len 大小的数据。
         * @param len
         */
        void makeSpace(size_t len);
    };
}

#endif //TINYWS_BUFFER_H
