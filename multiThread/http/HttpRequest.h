#ifndef TINYWS_HTTPREQUEST_H
#define TINYWS_HTTPREQUEST_H

#include <string>
#include <map>

#include "../net/Timer.h"

namespace tinyWS {
    // 用于保存请求相关的信息：请求方法、请求路径、查询字段、接收请求的时间、请求头
    class HttpRequest {
    public:
        // 请求方法
        enum Method {
            kInvalid, kGet, kPost, kHead, kPut, kDelete
        };

        HttpRequest();

        /**
         * 设置请求方法，并返回请求方法是否有效。
         * @param start 请求方法字符串的起始指针
         * @param end 请求方法字符串的末尾指针
         * @return 请求方法是否有效
         */
        bool setMethod(const char *start, const char *end);

        /**
         * 获取请求方法
         * @return 请求方法
         */
        Method method() const;

        /**
         * 获取请求方法的字符串
         * @return 请求方法字符串
         */
        const char* methodString() const;

        /**
         * 设置请求路径
         * @param start 请求路径字符串的起始指针
         * @param end 请求路径字符串的末尾指针
         */
        void setPath(const char *start, const char *end);

        /**
         * 获取请求路径
         * @return 请求路径字符串
         */
        const std::string& path() const;

        /**
         * 设置查询字段
         * @param start 查询字段字符串的起始指针
         * @param end 查询字段字符串的末尾指针
         */
        void setQuery(const char *start, const char *end);

        /**
         * 获取查询字段
         * @return 查询字段字符串
         */
        const std::string& query() const;

        /**
         * 设置请求接收时间
         * @param time 时间
         */
        void setReceiveTime(Timer::TimeType time);

        /**
         * 获取请求接收时间
         * @return 请求接收时间
         */
        Timer::TimeType receiveTime() const;

        /**
         * 添加请求头字段
         * @param start 请求头字段字符串的起始位置
         * @param colon 分隔符（:）的位置
         * @param end 请求头字段字符串的末尾位置
         */
        void addHeader(const char *start, const char *colon, const char *end);

        /**
         * 获取特定请求头字段的值
         * @param field 名字
         * @return 值
         */
        std::string getHeader(const std::string &field) const;

        /**
         * 获取所有请求头
         * @return 请求头映射 <key, value>
         */
        const std::map<std::string, std::string>& headers() const;

        // 交换
        void swap(HttpRequest &that);

    private:
        Method method_;                                 // 请求方法
        std::string path_;                              // 请求路径
        std::string query_;                             // 查询字段
        Timer::TimeType receiveTime_;                   // 请求接收时间
        std::map<std::string, std::string> headers_;    // 请求头映射 <key, value>
    };
}

#endif //TINYWS_HTTPREQUEST_H
