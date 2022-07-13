// @Author: Lawson
// @Date: 2022/03/19


#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>      //提供open,fcntl,fclose等文件控制
#include <stdio.h>
#include <sys/stat.h>   //获取文件属性
#include <unistd.h>     //POSIX系统调用的API

AppendFile::AppendFile(std::string filename)
    : fp_(fopen(filename.c_str(), "ae")) {  // e-O_CLOEXEC当调用exec成功后，文件会自动关闭
        // 设置文件流的缓冲区
        setbuffer(fp_, buffer_, sizeof buffer_);
  }

AppendFile::~AppendFile() { fclose(fp_); }

void AppendFile::append(const char* logline, const size_t len) {
    size_t n = this->write(logline, len);
    size_t remain = len - n;
    while(remain > 0) {
        size_t x = this->write(logline + n, remain);
        if(x == 0) {
            // 检查是否存在错误标识符
            int err = ferror(fp_);
            if(err) fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        n += x;
        remain = len - n;
    }
}

// 强迫将buffer内的数据写入指定的文件
void AppendFile::flush() { fflush(fp_); }

size_t AppendFile::write(const char* logline, size_t len) {
    // fwrite的线程不安全版，写入速度明显比fwrite快
    return fwrite_unlocked(logline, 1, len, fp_);
}



