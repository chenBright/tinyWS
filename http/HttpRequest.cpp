#include "HttpRequest.h"

#include <cassert>
#include <cctype>

#include <algorithm>

using namespace tinyWS;

HttpRequest::HttpRequest()
    : method_(kInvalid),
      receiveTime_(0) {

}

bool HttpRequest::setMethod(const char *start, const char *end) {
    assert(method_ == kInvalid);

    std::string m(start, end);
    if (m == "GET") {
        method_ = kGet;
    } else if (m == "POST") {
        method_ = kPost;
    } else if (m == "HEAD") {
        method_ = kHead;
    } else if (m == "PUT") {
        method_ = kPut;
    } else if (m == "DELETE") {
        method_ = kDelete;
    } else {
        method_ = kInvalid;
    }

    return method_ != kInvalid;
}

HttpRequest::Method HttpRequest::method() const {
    return method_;
}

const char* HttpRequest::methodString() const {
    const char *mStr = "UNKNOWN";
    switch (method_) {
        case kGet:
            mStr = "GET";
            break;
        case kPost:
            mStr = "POST";
            break;
        case kHead:
            mStr = "HEAD";
            break;
        case kPut:
            mStr = "PUT";
            break;
        case kDelete:
            mStr = "DELETE";
            break;
        default:
            break;
    }

    return mStr;
}

void HttpRequest::setPath(const char *start, const char *end) {
    path_.assign(start, end);
}

const std::string& HttpRequest::path() const {
    return path_;
}

void HttpRequest::setQuery(const char *start, const char *end) {
    query_.assign(start, end);
}

const std::string& HttpRequest::query() const {
    return query_;
}

void HttpRequest::setReceiveTime(Timer::TimeType time) {
    receiveTime_ = time;
}

Timer::TimeType HttpRequest::receiveTime() const {
    return receiveTime_;
}

void HttpRequest::addHeader(const char *start, const char *colon, const char *end) {
    std::string field(start, colon);
    ++colon;
    // 跳过空格
    while (colon < end && ::isspace(*colon)) {
        ++colon;
    }

    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1])) {
        value.resize(value.size() - 1);
    }
    headers_[field] = value;
}

std::string HttpRequest::getHeader(const std::string &field) const {
    std::string value;
    auto it = headers_.find(field);
    if (it != headers_.end()) {
        value = it->second;
    }

    return value;
}

const std::map<std::string, std::string>& HttpRequest::headers() const {
    return headers_;
}

void HttpRequest::swap(HttpRequest &that) {
    std::swap(method_, that.method_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    std::swap(receiveTime_, that.receiveTime_);
    headers_.swap(that.headers_);
}
