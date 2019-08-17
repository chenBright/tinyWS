#ifndef TINYWS_HTTPRESPONSE_H
#define TINYWS_HTTPRESPONSE_H

#include <map>
#include <string>

namespace tinyWS {
    class Buffer;

    class HttpResponse {
    public:
        enum HttpStatusCode {
            kUnknown,
            k200OK = 200,
            k301MovedPermanently = 301,
            k400BadRequest  = 400,
            k404NotFound = 404
        };

        explicit HttpResponse(bool close);

        void setStatusCode(HttpStatusCode code);
        void setStatusMessage(const std::string &message);
        void setCloseConnection(bool on);
        bool closeConnection() const;
        void setContentType(const std::string &contentType);
        void addHeader(const std::string &key, const std::string & value);
        void setBody(const std::string &body);
        void appendToBuffer(Buffer *output) const;

    private:
        std::map<std::string, std::string> headers_;
        HttpStatusCode statusCode_;
        std::string statusMessage_;
        bool closeConnection_;
        std::string body_;
    };
}

#endif //TINYWS_HTTPRESPONSE_H
