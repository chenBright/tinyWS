#ifndef TINYWS_BUFFER_H
#define TINYWS_BUFFER_H

#include <vector>
#include <string>

#include "base/noncopyable.h"

namespace tinyWS {
    class Buffer : public tinyWS::noncopyable {
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

        Buffer();

        // 使用默认的拷贝构造函数、赋值函数和析构函数

        void swap(Buffer &rhs);

        size_t readableBytes() const;
        size_t writableBytes() const;
        size_t prependableBytes() const;
        const char* peek() const;


        void retrieve(size_t len);
        void retrieveUntil(const char *end);
        // Read int32_t from network endian
        void retrieveInt32();
        void retrieveInt16();
        void retrieveInt8();
        void retrieveAll();
        std::string retrieveAllAsString();
        std::string retrieveAsString(size_t len);

        std::string toString() const;

        void append(const std::string &str);
        void append(const char *data, size_t len);
        void append(const void *data, size_t len);

        void ensureWritableBytes(size_t len);

        char* beginWrite();
        const char* beginWrite() const;

        /**
         * 写入了数据，更新写指针
         * @param len 写入数据的长度
         */
        void hasWritten(size_t len);

        void appendInt32(int32_t x);
        void appendInt16(int16_t x);
        void appendInt8(int8_t x);

        int32_t readInt32();
        int16_t readInt16();
        int8_t readInt8();

        int32_t peekInt32() const;
        int16_t peekInt16() const;
        int8_t peekInt8() const;

        void prependInt32(int32_t x);
        void prependInt16(int16_t x);
        void prependInt8(int8_t x);

        void prepend(const void *data, size_t len);

        void shrink(size_t reserve);

        ssize_t readFd(int fd, int *savedErrno);



    private:
        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;

        char * begin();
        const char* begin() const;
        void makeSpace(size_t len);
    };
}


#endif //TINYWS_BUFFER_H
