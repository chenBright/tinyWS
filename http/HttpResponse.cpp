#include "HttpResponse.h"

#include <cstdio>

#include "../net/Buffer.h"

using namespace tinyWS;

HttpResponse::HttpResponse(bool close)
    : statusCode_(kUnknown),
      closeConnection_(close){

}

void HttpResponse::setStatusCode(HttpStatusCode code) {
    statusCode_ = code;
}

void HttpResponse::setStatusMessage(const std::string &message) {
    statusMessage_ = message;
}

void HttpResponse::setCloseConnection(bool on) {
    closeConnection_ = on;
}

bool HttpResponse::closeConnection() const {
    return closeConnection_;
}

void HttpResponse::setContentType(const std::string &contentType) {
    addHeader("Content-Type", contentType);
}

void HttpResponse::addHeader(const std::string &key, const std::string &value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::string &body) {
    body_ = body;
}

void HttpResponse::appendToBuffer(Buffer *output) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", statusCode_);

    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if (closeConnection_) {
        output->append("Connection: close\r\n");
    } else {
        snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto &header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);
}
