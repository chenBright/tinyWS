#ifndef TINYWS_HTTPCONTEXT_H
#define TINYWS_HTTPCONTEXT_H

#include "HttpRequest.h"
#include "Timer.h"

namespace tinyWS {
    class Buffer;

    class HttpContext {
    public:
        enum HttpRequestParseState {
            kExpectRequestLine,
            kExpectHeader,
            kExpectBody,
            kGotAll
        };

        HttpContext();

        bool parseRequest(Buffer *buffer, Timer::TimeType receiveTime);

        bool gotAll() const;

        void reset();

        const HttpRequest& request() const;

        HttpRequest& request();

    private:
        HttpRequestParseState state_;
        HttpRequest request_;

        bool processRequestLine(const char *start, const char *end);
    };
}


#endif //TINYWS_HTTPCONTEXT_H
