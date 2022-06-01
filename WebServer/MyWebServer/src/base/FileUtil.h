// @Author: Lawson
// @Date: 2022/03/19

#pragma once
#include <string>
#include "noncopyable.h"


// FileUtil是最底层的文件类，封装了log文件的打开和写入
// 在类析构时关闭文件，底层使用了标准IO
class AppendFile : noncopyable {
public:
    explicit AppendFile(std::string filename);
    ~AppendFile();
    // append会向文件写
    void append(const char* logline, const size_t len);
    void flush();


private:
    size_t write(const char* logline, size_t len);
    FILE *fp_;
    char buffer_[64 * 1024];
};
