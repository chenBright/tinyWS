#ifndef TINYWS_HTTPRESPONSE_H
#define TINYWS_HTTPRESPONSE_H

#include <string>
#include <map>

namespace tinyWS_process2 {

    class Buffer;

    class HttpResponse {
    public:
        // 状态码
        enum HttpStatusCode {
            kUnknown,
            k200OK = 200,
            k301MovedPermanently = 301,
            k400BadRequest  = 400,
            k404NotFound = 404
        };

    private:
        std::map<std::string, std::string> headers_;    // 响应头映射 <key, value>
        HttpStatusCode statusCode_;                     // 状态码
        std::string statusMessage_;                     // 状态信息
        bool closeConnection_;                          // 是否将 Connection 字段设置为 close
        std::string body_;                              // Response Body

    public:
        /**
         * 构造函数
         * @param close 连接是否已关闭
         */
        explicit HttpResponse(bool close);

        /**
         * 设置状态码
         * @param code 状态码
         */
        void setStatusCode(HttpStatusCode code);

        /**
         * 设置状态信息
         * @param message 状态信息
         */
        void setStatusMessage(const std::string& message);

        /**
         * 设置是否将 Connection 字段设置为 close
         * @param on true / false
         */
        void setCloseConnection(bool on);

        /**
         * 获取是否已将 Connection 字段设置为 close
         * @return true / false
         */
        bool closeConnection() const;

        /**
         * 设置 Content-Type
         * @param contentType Content-Type 字符串
         */
        void setContentType(const std::string& contentType);

        /**
         * 添加响应头
         * @param key
         * @param value
         */
        void addHeader(const std::string& key, const std::string& value);

        /**
         * 设置 Response Body
         * @param body Response Body 字符串
         */
        void setBody(const std::string& body);

        /**
         * 将响应数据（包括响应头和 Body）添加到 Buffer 中
         * @param output 数据指针
         */
        void appendToBuffer(Buffer* output) const;
    };
}


#endif //TINYWS_HTTPRESPONSE_H
