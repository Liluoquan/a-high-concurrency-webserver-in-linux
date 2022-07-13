#ifndef LOG_LOGGING_H_
#define LOG_LOGGING_H_

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "log_stream.h"

namespace log {
class Logging {
 public:
    Logging(const char* file_name, int line, int level);
    ~Logging();

    static std::string log_file_name() {
        return log_file_name_;
    }

    static void set_log_file_name(std::string log_file_name) {
        log_file_name_ = log_file_name;
    }

    static bool open_log() {
        return open_log_;
    }

    static void set_open_log(bool open_log) {
        open_log_ = open_log;
    }

    static void set_log_to_stderr(bool log_to_stderr) {
        log_to_stderr_ = log_to_stderr;
    }

    static void set_color_log_to_stderr(bool color_log_to_stderr) {
        color_log_to_stderr_ = color_log_to_stderr;
    }

    static void set_min_log_level(int min_log_level) {
        min_log_level_ = min_log_level;
    }

    LogStream& stream() {
        return impl_.stream_;
    }

 private:
    class Impl {
     public:
        Impl(const char* file_name, int line, int level);
        void FormatLevel();
        void FormatTime();

        LogStream stream_;
        std::string file_name_;
        int line_;
        int level_;
        std::string level_str_;
        std::string log_color_;
        char time_str_[26];
        bool is_fatal_;
    };

 private:
    static std::string log_file_name_;
    static bool open_log_;
    static bool log_to_stderr_;
    static bool color_log_to_stderr_;
    static int min_log_level_;

    Impl impl_;
};

}  // namespace log

enum LogLevel {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

//宏定义
#ifdef OPEN_LOGGING
    #define LOG(level) log::Logging(__FILE__, __LINE__, level).stream() 
    #define LOG_DEBUG   log::Logging(__FILE__, __LINE__, DEBUG).stream()
    #define LOG_INFO    log::Logging(__FILE__, __LINE__, INFO).stream()
    #define LOG_WARNING log::Logging(__FILE__, __LINE__, WARNING).stream()
    #define LOG_ERROR   log::Logging(__FILE__, __LINE__, ERROR).stream()
    #define LOG_FATAL   log::Logging(__FILE__, __LINE__, FATAL).stream()
#else
    #define LOG(level) log::LogStream()
#endif

#endif  // LOG_LOGGING_H_
