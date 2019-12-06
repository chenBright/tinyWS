#include "HttpContext.h"

#include <algorithm>

#include "../net/Buffer.h"

using namespace tinyWS_process2;

HttpContext::HttpContext() : state_(kExpectRequestLine) {

}


bool HttpContext::parseRequest(Buffer* buffer, TimeType receiveTime) {
    bool isOk = true;
    bool hasMore = true;
    while (hasMore) {
        if (state_ == kExpectRequestLine) {
            // 解析请求行

            const char *crlf = buffer->findCRLF(); // 查找"r\n"
            if (crlf) {
                // 查找成功，当前请求行完整
                isOk = processRequestLine(buffer->peek(), crlf);

                if (isOk) {
                    // 行解析成功
                    request_.setReceiveTime(receiveTime);
                    // 更新 Buffer
                    buffer->retrieveUntil(crlf + 2);
                    // 行解析成功，下一步是解析请求头
                    state_ = kExpectHeader;
                } else {
                    // 请求行解析失败
                    hasMore = false;
                }
            } else {
                // 请求行不完整
                hasMore = false;
            }
        } else if (state_ == kExpectHeader) {
            // 解析请求头

            const char *crlf = buffer->findCRLF(); // 查找"r\n"
            if (crlf) {
                // 查找成功，当前有一行完整的数据
                const char *colon = std::find(buffer->peek(), crlf, ':'); // 查找":"
                if (colon != crlf) {
                    request_.addHeader(buffer->peek(), colon, crlf);
                } else {
                    // 空行（"r\n"），请求头结束
                    state_ = kGotAll;
                    hasMore = false;
                }
                // 更新 Buffer
                buffer->retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        } else if (state_ == kExpectBody) {
            // TODO
        }
    }

    return isOk;
}

bool HttpContext::gotAll() const {
    return state_ == kGotAll;
}

void HttpContext::reset() {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
}

const HttpRequest& HttpContext::request() const {
    return request_;
}

HttpRequest& HttpContext::request() {
    return request_;
}

bool HttpContext::processRequestLine(const char* start, const char* end) {
    bool isSucceed = false;

    // 请求方法 路径 HTTP/版本
    const char *space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            const char *question = std::find(start, space, '?');
            if (question != space) {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            } else {
                request_.setPath(start, space);
            }
        }

        start = space + 1;
        // 8个字节：HTTP/1.1
        isSucceed = end - start == 8 && std::equal(start, end, "HTTP/1.1");
    }

    return isSucceed;
}
