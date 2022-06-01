// @Author: Lawson
// @Date: 2022/03/22

#include "LogStream.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <limits>

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;

// 该函数将整数转换为字符串
template<typename T>
size_t convert(char buf[], T value) {
    T i = value;
    char *p = buf;

    do{
        // lsd-last digit
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd]; //注：此处lsd是可以为负的
    } while(i != 0);

    if(value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf; //返回buf的长度
}


template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;


template <typename T>
void LogStream::formatInteger(T v) {
    // buffer容不下kMaxNumericSize个字符会被直接丢弃
    if(buffer_.avail() >= kMaxNumericSize) {
        // 直接将buffer的当前可用位置赋进去
        // 然后再移动buffer的当前位置
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}


// 能转换为int的都弄成int处理
LogStream& LogStream::operator<< (short v) {
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<< (unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<< (int v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<< (unsigned int v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<< (long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<< (unsigned long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<< (long long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<< (unsigned long long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<< (double v) {
    if(buffer_.avail() >= kMaxNumericSize) {
        //"%.12g"-以小数/指数中的较短值输出，精度为12位
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<< (long double v) {
    if(buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v);
        buffer_.add(len);
    }
    return *this;
}






