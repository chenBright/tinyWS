#ifndef TINYWS_HTTPREQUEST_H
#define TINYWS_HTTPREQUEST_H

#include <string>
#include <map>

#include "Timer.h"

namespace tinyWS {
    class HttpRequest {
    public:
        enum Method {
            kInvalid, kGet, kPost, kHead, kPut, kDelete
        };

        HttpRequest();

        bool setMethod(const char *start, const char *end);
        Method method() const;
        const char* methodString() const;
        void setPath(const char *start, const char *end);
        const std::string& path() const;
        void setQuery(const char *start, const char *end);
        const std::string& query() const;
        void setReceiveTime(Timer::TimeType time);
        Timer::TimeType receiveTime() const;

        void addHeader(const char *start, const char *colon, const char *end);
        std::string getHeader(const std::string &field) const;
        const std::map<std::string, std::string>& headers() const;

        void swap(HttpRequest &that);

    private:
        Method method_;
        std::string path_;
        std::string query_;
        Timer::TimeType receiveTime_;
        std::map<std::string, std::string> headers_;
    };
}

#endif //TINYWS_HTTPREQUEST_H
