// @Author: Lawson
// @Date: 2022/03/21

#include "LogFile.h"
#include "FileUtil.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace std;

LogFile::LogFile(const string& basename, int flushEveryN)
    : basename_(basename),
      flushEveryN_(flushEveryN), 
      count_(0), 
      mutex_(new MutexLock) {
    assert(basename.find('/') >= 0);
    // unique_ptr::reset(p) : 获取p所指向的堆内存的所有权
    file_.reset(new AppendFile(basename));
}

LogFile::~LogFile() { }

void LogFile::append(const char* logline, int len) {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
}

void LogFile::flush() {
    MutexLockGuard lock(*mutex_);
    file_->flush();
}

void LogFile::append_unlocked(const char* logline, int len) {
    file_->append(logline, len);
    ++count_;
    // 每写入1024行，强制刷新一次buffer（将buffer写入文件） 
    if(count_ >= flushEveryN_) {
        count_ = 0;
        file_->flush();
    }
}






