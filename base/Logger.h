#ifndef TINYWS_LOGGER_H
#define TINYWS_LOGGER_H

#include <ctime>

#include <string>
#include <sstream>
#include <fstream>

#include "MutexLock.h"
#include "noncopyable.h"

namespace tinyWS {
    // 参考 https://blog.csdn.net/u014755412/article/details/79334572

    // 日志等级
    // 使用枚举类来避免污染命名空间。
    enum class LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    // 纯虚日志基类
    class BaseLogger {
        class LogStream; // 用于文本缓冲的内部类声明
    public:

        BaseLogger() = default;

        virtual ~BaseLogger() = default;

        /**
         * 重载 operator()，使日志实例变为可调用。
         * @param level 日志等级
         * @return 缓冲区对象
         */
        virtual LogStream operator()(LogLevel level = LogLevel::DEBUG);

    private:
        MutexLock mutex_;
        tm localtime_{};

        /**
         * 获取当前本地时间
         * @return 本地时间
         */
        const tm* getLocalTime();

        /**
         * --- 线程安全 ---
         * @param level 日志等级
         * @param message 打印的信息
         */
        void endine(LogLevel level, const std::string &message);

        /**
         * 纯虚函数，打印日志。
         * @param tmPtr 本地时间
         * @param levelStr 日志等级
         * @param messageStr 打印的信息
         */
        virtual void output(const tm *tmPtr, const std::string &levelStr, const std::string &messageStr) = 0;
    };

    class BaseLogger::LogStream : public std::ostringstream {
    public:
        LogStream(BaseLogger &logger, LogLevel level);
        LogStream(const LogStream &other);
        ~LogStream() override;
    private:
        BaseLogger &logger_;
        LogLevel level_;
    };

    // 控制台日志类
    class ConsoleLogger : public BaseLogger {
        using BaseLogger::BaseLogger;
        void output(const tm *tmPtr, const std::string &levelStr, const std::string &messageStr) override;
    };

    // 文档日志类
    class FileLogger
            : public BaseLogger,
              private noncopyable {
    public:
        explicit FileLogger(std::string filename) noexcept;

        ~FileLogger() override;
    private:
        std::ofstream file_;

        void output(const tm *tmPtr, const std::string &levelStr, const std::string &messageStr) override;
    };

    // 全局实例
    extern ConsoleLogger debug;
//    extern FileLogger record;
}


#endif //TINYWS_LOGGER_H
