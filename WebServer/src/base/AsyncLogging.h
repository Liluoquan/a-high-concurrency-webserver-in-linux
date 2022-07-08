// Author: Lawson
// Date: 2022/03/21

#pragma once
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>


// AsyncLogging负责启动一个log线程，专门用来将log写入LogFile，应用了“双缓冲技术”
class AsyncLogging : noncopyable {
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging() {
        if(running_) stop();
    }
    void append(const char* logline, int len);

    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() {
        running_ = false;
        cond_.notify();
        thread_.join();
    }
        
private:
    void threadFunc();  // 后端日志线程函数，用于把缓冲区日志写入文件
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    typedef std::shared_ptr<Buffer> BufferPtr;

    const int flushInterval_;       //超时时间，每隔一段时间写日志
    bool running_;                  //FIXME:std::atomic<bool> running_;
    const std::string basename_;    //日志名字
    Thread thread_;                 //后端线程，用于将日志写入文件
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
    CountDownLatch latch_;          //倒计时，用于指示日志记录器何时开始工作
};



