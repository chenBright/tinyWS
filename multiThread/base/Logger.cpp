#include "Logger.h"

#include <cassert>
#include <chrono>

#include <map>
#include <iostream>
#include <iomanip>
#include <regex>

using namespace tinyWS_thread;

ConsoleLogger tinyWS_thread::debug;
//FileLogger tinyWS::record("test.log");

static const std::map<LogLevel, const std::string> LogLevelStr =
        {
            {LogLevel::TRACE, "TRACE"},
            {LogLevel::DEBUG, "DEBUG"},
            {LogLevel ::INFO, "INFO"},
            {LogLevel ::WARN, "WARN"},
            {LogLevel::ERROR, "ERROR"},
            {LogLevel::FATAL, "FATAL"}
        };

std::ostream& operator<<(std::ostream &stream, const tm *tmStr) {
    return stream << 1900 + tmStr->tm_year << '-'
                  << std::setfill('0') << std::setw(2) << tmStr->tm_mon + 1 << '-'
                  << std::setfill('0') << std::setw(2) << tmStr->tm_mday << ' '
                  << std::setfill('0') << std::setw(2) << tmStr->tm_hour << ':'
                  << std::setfill('0') << std::setw(2) << tmStr->tm_min << ':'
                  << std::setfill('0') << std::setw(2) << tmStr->tm_sec;

}

BaseLogger::LogStream BaseLogger::operator()(LogLevel level) {
    return LogStream(*this, level);
}

const tm* BaseLogger::getLocalTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    localtime_r(reinterpret_cast<const time_t *>(&in_time_t), &localtime_);

    return &localtime_;
}

void BaseLogger::endine(LogLevel level, const std::string &message) {
    MutexLockGuard lock(mutex_);
    output(getLocalTime(), LogLevelStr.find(level)->second, message);
}

BaseLogger::LogStream::LogStream(BaseLogger &logger, tinyWS_thread::LogLevel level)
    : logger_(logger),
      level_(level) {

}

BaseLogger::LogStream::LogStream(const BaseLogger::LogStream &other)
    : logger_(other.logger_),
      level_(other.level_) {

}

BaseLogger::LogStream::~LogStream() {
    // 在析构的时候，打印日志
    logger_.endine(level_, static_cast<std::string>(std::move(str())));
}

void ConsoleLogger::output(const tm *tmPtr, const std::string &levelStr, const std::string &messageStr) {
    std::cout << '[' << tmPtr << ']'
              << '[' << levelStr << ']'
              << '\t' << messageStr << std::endl;
}

FileLogger::FileLogger(std::string filename) noexcept : BaseLogger() {
    std::string validFilename(filename.size(), '\0');
    std::regex express(R"(/|:| |>|<|\"|\\*|\\?|\\|)");
    std::regex_replace(validFilename.begin(), filename.begin(), filename.end(), express, "-");
    file_.open(validFilename, std::fstream::out | std::fstream::app | std::fstream::ate);

    assert(!file_.fail());
}

FileLogger::~FileLogger() {
    file_.flush();
    file_.close();
}

void FileLogger::output(const tm *tmPtr, const std::string &levelStr, const std::string &messageStr) {
    file_ << '[' << tmPtr << ']'
          << '[' << levelStr << ']'
          << '\t' << messageStr << std::endl;
    file_.flush();
}