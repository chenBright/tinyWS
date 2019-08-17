#include "HttpContext.h"

#include <algorithm>

#include "../net/Buffer.h"

using namespace tinyWS;

HttpContext::HttpContext() : state_(kExpectRequestLine) {

}
// return false if any error
bool HttpContext::parseRequest(Buffer *buffer, Timer::TimeType receiveTime) {
    bool isOk = true;
    bool hasMore = true;
    while (hasMore) {
        if (state_ == kExpectRequestLine) {
            const char *crlf = buffer->findCRLF();
            if (crlf) {
                isOk = processRequestLine(buffer->peek(), crlf);
                if (isOk) {
                    request_.setReceiveTime(receiveTime);
                    buffer->retrieveUntil(crlf + 2);
                    state_ = kExpectHeader;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if (state_ == kExpectHeader) {
            const char *crlf = buffer->findCRLF();
            if (crlf) {
                const char *colon = std::find(buffer->peek(), crlf, ':');
                if (colon != crlf) {
                    request_.addHeader(buffer->peek(), colon, crlf);
                } else {
                    // empty line, end of header
                    state_ = kGotAll;
                    hasMore = false;
                }
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

bool HttpContext::processRequestLine(const char *start, const char *end) {
    bool isSucceed = false;
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
        isSucceed = end - start == 8 && std::equal(start, end, "HTTP/1.1");
    }

    return isSucceed;
}
