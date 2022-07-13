
#include "log/logging.h"

#include <assert.h>
#include <sys/time.h>
#include <time.h>

#include <iostream>

#include "log/async_logging.h"
#include "thread/thread.h"

namespace log {
//异步日志对象 异步日志线程
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static AsyncLogging* async_logging = nullptr;

//日志的静态变量
std::string Logging::log_file_name_ = "./web_server.log";
bool Logging::open_log_ = true;
bool Logging::log_to_stderr_ = false;
bool Logging::color_log_to_stderr_ = false;
int  Logging::min_log_level_ = DEBUG;

//初始化异步日志 并start线程
void OnceInit() {
    async_logging = new AsyncLogging(Logging::log_file_name());
    async_logging->Start();
}

//运行异步日志线程(只在第一次调用时运行,将buffer数据写入日志文件), 写日志到buffer中
void WriteLog(const char* single_log, int size, bool is_fatal) {
    //只会执行一次的函数
    pthread_once(&once_control, OnceInit);
    // 写日志到buffer中
    async_logging->WriteLog(single_log, size, is_fatal);
}

//logging
Logging::Logging(const char* file_name, int line, int level) 
    : impl_(file_name, line, level) {
    //在logging类对象构造时将日志(时间，文件名，行，日志等级)写到stream的buffer中
    impl_.stream_ << impl_.time_str_ << " " 
                  << impl_.file_name_ << ':' 
                  << impl_.line_ << impl_.level_str_;
}

Logging::~Logging() {
    if (impl_.level_ >= min_log_level_) {
        //调用LOG时 <<会的日志内容会追加写入stream的buffer中,最后加一个换行符
        //buffer通过日志的异步线程写入日志文件
        impl_.stream_ << "\n";
        const auto& buffer = stream().buffer();

        //写日志
        if (open_log) {
            WriteLog(buffer.buffer(), buffer.size(), impl_.is_fatal_);
        }
        //日志输出到标准错误流
        if (log_to_stderr_) {
            if (color_log_to_stderr_) {
                std::cout << impl_.log_color_
                        << std::string(buffer.buffer(), buffer.size()) 
                        << COLOR_END;
            } else {
                std::cout << std::string(buffer.buffer(), buffer.size());
            }
        }
        
        if (impl_.is_fatal_) {
            abort();
        }
    }
}

Logging::Impl::Impl(const char* file_name, int line, int level)
    : stream_(), 
      file_name_(file_name), 
      line_(line),
      level_(level),
      is_fatal_(false) {
    //文件名只取最后面的
    auto pos = file_name_.find_last_of("/");
    if (pos != file_name_.npos) {
        file_name_ = file_name_.substr(pos + 1);
    }
    FormatLevel();
    FormatTime();
}

//写日志等级到stream的buffer中
void Logging::Impl::FormatLevel() {
    switch (level_) {
        case DEBUG:
            level_str_ = " [Debug]: ";
            log_color_ = GRAY;
            break;
        case INFO:
            level_str_ = " [Info]: ";
            log_color_ = BRIGHT_GREEN;
            break;
        case WARNING:
            level_str_ = " [Warning]: ";
            log_color_ = BOLD;
            break;
        case ERROR:
            level_str_ = " [Error]: ";
            log_color_ = BRIGHT_RED;
            break;
        case FATAL:
            level_str_ = " [Fatal]: ";
            log_color_ = RED;
            break;
        default:
            level_str_ = " [Info]: ";
            log_color_ = GRAY;
            is_fatal_ = true;
            break;
    }
}

//写当前时间到stream的buffer中
void Logging::Impl::FormatTime() {
    //gettimeofday得到当前时间 timeval有两个成员 一个秒 一个微秒
    //time函数一样的效果 返回一个time_t类型表示秒 然后使用localtime(&time_t)获得当前时间 类型为tm
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm* current_time = localtime(&now.tv_sec);

    //%m:十进制的月份 %H：24小时制的小时 %M:十时制的分钟
    strftime(time_str_, 26, "%Y-%m-%d %H:%M:%S", current_time);
}

}  // namespace log
