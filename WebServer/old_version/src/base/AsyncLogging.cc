// @Author: Lawson
// @Date: 2022/03/23

#include "AsyncLogging.h"
#include "LogFile.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>

AsyncLogging::AsyncLogging(std::string logFileName, int flushInterval)
    : flushInterval_(flushInterval), 
      running_(false), 
      basename_(logFileName),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1) {
    assert(logFileName.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len) {
    //操作缓冲区需要加锁
    MutexLockGuard lock(mutex_);
    //剩余空间够则直接写入当前buffer
    if(currentBuffer_->avail() > len)
        currentBuffer_->append(logline, len);
    else {  //否则换nextbuffer来写
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        if(nextBuffer_)
            currentBuffer_ = std::move(nextBuffer_);
        else    //如果nextbuffer不可用，则新建一个buffer来写
            currentBuffer_.reset(new Buffer);
        currentBuffer_->append(logline, len);
        //通知buffers_中有数据了，可以往文件中写了
        cond_.notify();
    }
}

void AsyncLogging::threadFunc() {
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);  //LogFile用于将日志写入文件
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;    //该vector属于后端线程，用于和前端的buffers进行交换
    buffersToWrite.reserve(16);

    while(running_) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        //将前端buffers_中的数据交换到buffersToWrite中
        {
            MutexLockGuard lock(mutex_);
            //每隔3s,或者currentBuffer满了，就将currentBuffer放入buffers_中
            if(buffers_.empty())
                cond_.waitForSeconds(flushInterval_);
            
            buffers_.push_back(std::move(currentBuffer_));
            // buffers_.push_back(currentBuffer_);
            // currentBuffer_.reset();
            currentBuffer_ = std::move(newBuffer1);

            buffersToWrite.swap(buffers_);
            if(!nextBuffer_)
                nextBuffer_ = std::move(newBuffer2);
        }
    

        assert(!buffersToWrite.empty());

        //如果队列中buffer数目大于25，就删除多余数据
        //避免日志堆积：前端日志记录过快，后端来不及写入文件
        if(buffersToWrite.size() > 25) {
            //TODO:删除数据时加错误提示
            //只留原始的两个buffer，其余的删除
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for(const auto& buffer : buffersToWrite) {
            output.append(buffer->data(), buffer->length());
        }

        //重新调整buffersToWrite的大小，仅保留两个原始buffer
        if(buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        if(!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if(!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}





