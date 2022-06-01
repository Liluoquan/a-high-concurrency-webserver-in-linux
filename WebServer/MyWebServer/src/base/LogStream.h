// Author: Lawson
// Date: 2022/03/21

#pragma once
#include <assert.h>
#include <string.h>         //c中针对char*的函数
#include <string>           //c++中的string类
#include "noncopyable.h"

class AsyncLogging;
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE> //非类型模板参数，相当于在编译器就给常量赋值
class FixedBuffer : noncopyable {
public:
    FixedBuffer() 
        : cur_(data_) {
        //TODO 加入setcookie的功能
    }

    ~FixedBuffer() {}

    void append(const char* buf, size_t len) {
        if(avail() > static_cast<int>(len)) { //如果可用数据足够，就拷贝过去，并移动当前指针
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }  //返回首地址
    int length() const { return static_cast<int>(cur_ - data_); }  //返回缓冲区已有数据长度

    char* current() { return cur_; }    //返回当前数据末端地址
    int avail() const { return static_cast<int>(end() - cur_); }    //返回剩余可用地址
    void add(size_t len) { cur_ += len; }   //cur前移

    void reset() { cur_ = data_; }  //重置不清除数据，仅需让cur指回首地址
    void bzero() { memset(data_, 0, sizeof data_); }    //数据清零
private:
    const char* end() const { return data_ + sizeof data_; }    //返回尾指针

    char data_[SIZE];
    char* cur_; //指向已有数据的最右端,类似data_->cur_->end()
};

class LogStream : noncopyable {
    typedef LogStream self;
public:
    typedef FixedBuffer<kSmallBuffer> Buffer;

    self& operator<< (bool v) {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<< (short);
    self& operator<< (unsigned short);
    self& operator<< (int);
    self& operator<< (unsigned int);
    self& operator<< (long);
    self& operator<< (unsigned long);
    self& operator<< (long long);
    self& operator<< (unsigned long long);

    self& operator<< (const void*);

    self& operator<< (float v) {
        *this << static_cast<double>(v);
        return *this;
    }
    self& operator<< (double);
    self& operator<< (long double);

    self& operator<< (char v) {
        buffer_.append(&v, 1);
        return *this;
    }

    self& operator<< (const char* str) {
        if(str)
            buffer_.append(str, strlen(str));
        else
            buffer_.append("(null)", 6);
        return *this;
    }

    self& operator<< (const unsigned char* str) {
        // reinterpret_cast用在任意指针(或引用)类型之间的转换
        return operator<<(reinterpret_cast<const char*>(str));
    }

    self& operator<< (const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }


    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }
private:
    void staticCheck();

    template <typename T>
        void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize = 32;
};







