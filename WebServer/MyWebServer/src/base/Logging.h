// @Author: Lawson
// @Date: 2022/03/23

#pragma once
#include "LogStream.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>

class AsyncLogging;

class Logger {
public:
    Logger(const char* filename, int line);
    ~Logger();
    LogStream& stream() { return impl_.stream_; }

    static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
    static std::string getLogFileName() { return logFileName_; }
private:
    //Logger的内部实现类Impl：负责把日志头信息写入logstream
    //包含：时间戳，LogStream数据流，源文件行号，源文件名字
    //TODO:加入日志级别
    class Impl {
     public:
         Impl(const char* fileName, int line);
         void formatTime();

         LogStream stream_;
         int line_;
         std::string basename_;
    };
    Impl impl_;
    static std::string logFileName_;
};


// C/C++提供了三个宏来定位程序运行时的错误
// __FUNCTION__:返回当前所在的函数名
// __FILE__:返回当前的文件名
// __LINE__:当前执行行所在行的行号
#define LOG Logger(__FILE__, __LINE__).stream()

