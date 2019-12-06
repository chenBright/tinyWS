#ifndef TINYWS_HTTPCONTEXT_H
#define TINYWS_HTTPCONTEXT_H

#include "HttpRequest.h"
#include "../net/type.h"

namespace tinyWS_process2 {

    class Buffer;

    class HttpContext {
    public:
        // 解析状态
        enum HttpRequestParseState {
            kExpectRequestLine, // 解析行
            kExpectHeader,      // 解析请求头
            kExpectBody,        // 解析 Body
            kGotAll             // 解析完成
        };

    private:
        HttpRequestParseState state_;
        HttpRequest request_;

    public:
        HttpContext();

        /**
         * 解析 Buffer 的数据，并将相应的信息添加到 HttpRequest 中，最后返回是否解析成功。
         * @param buffer 缓冲区
         * @param receiveTime 连接接收时间
         * @return 是否解析成功
         */
        bool parseRequest(Buffer* buffer, TimeType receiveTime);

        /*
         * 是否解析完成
         */
        bool gotAll() const;

        /**
         * 重置解析状态为 kExpectRequestLine，清空 HttpRequest
         */
        void reset();

        /**
         * 获取  HttpRequest
         * @return HttpRequest
         */
        const HttpRequest& request() const;

        // 同上
        HttpRequest& request();

    private:
        bool processRequestLine(const char* start, const char* end);

    };

}

#endif //TINYWS_HTTPCONTEXT_H
