// @Author: Lawson
// @Date: 2022/03/19

#pragma once
#include <memory>
#include <string>
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"

// TODO 提供自动归档功能
// TODO 增加日志滚动功能：
//      1、每写满1G换下个文件
//      2、每天零点新建一个日志文件，不论前一个文件是否写满
class LogFile : noncopyable {
public:
    // 每被append flushEveryN次，flush一下，会往文件写，当然是带缓冲区的 
    // 默认分割行数1024
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);

    const std::string basename_;
    const int flushEveryN_;

    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;
};
