#ifndef LOG_LOG_STREAM_H_
#define LOG_LOG_STREAM_H_

#include <assert.h>
#include <string.h>
#include <string>

#include "utility/noncopyable.h"
#include "locker/mutex_lock.h"

#define COLOR_END            "\e[0m"
#define BLACK                "\e[0;30m"
#define BRIGHT_BLACK         "\e[1;30m"
#define RED                  "\e[0;31m"
#define BRIGHT_RED           "\e[1;31m"
#define GREEN                "\e[0;32m"
#define BRIGHT_GREEN         "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define BRIGHT_BLUE          "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define BRIGHT_PURPLE        "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define BRIGHT_CYAN          "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K"

namespace log {
//类的前置声明
class AsyncLogging;
constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

//固定的缓冲区
template <int buffer_size>
class FixedBuffer : utility::NonCopyAble {
 public:
    FixedBuffer() 
        : size_(0) {
        bzero();
        current_buffer_ = buffer_;
    }

    ~FixedBuffer() {
    }

    //copy buffer的size个字节到current_buffer_来
    void write(const char* buffer, int size) {
        if (capacity() > size) {
            memcpy(current_buffer_, buffer, size);
            this->add(size);
        }
    }

    //源buffer
    const char* buffer() const {
        return buffer_;
    }

    //当前buffer(buffer+偏移量)
    char* current_buffer() {
        return current_buffer_;
    }

    //当前buufer的偏移量（相对buffer起始地址的偏移量）
    int size() const {
        return size_;
    }

    //剩余buffer偏移量 = 总buffer的偏移量 - 当前buffer的偏移量
    int capacity() const {
        return buffer_size - size_;
    }

    //当前buffer偏移size个字节
    void add(size_t size) {
        current_buffer_ += size;
        size_ += size;
    }

    //将当前buffer偏移量置为0,即为buffer
    void reset() {
        current_buffer_ = buffer_;
        size_ = 0;
    }

    //给buffer置0
    void bzero() {
        memset(buffer_, 0, sizeof(buffer_));
    }

 private:
    char buffer_[buffer_size];
    char* current_buffer_;
    int size_;
};

//输出流对象 重载输出流运算符<<  将值写入buffer中 
class LogStream : utility::NonCopyAble {
 public:
    using Buffer = FixedBuffer<kSmallBufferSize>;

    LogStream() {
    }
    ~LogStream() {
    }

    //重载输出流运算符<<  将值写入buffer中 
    LogStream& operator<<(bool express);

    LogStream& operator<<(short number);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float number);
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char str);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string&);

    void Write(const char* buffer, int size) {
        buffer_.write(buffer, size);
    }

    const Buffer& buffer() const {
        return buffer_;
    }

    void reset_buffer() {
        buffer_.reset();
    }

 private:
    template <typename T>
    void FormatInt(T);

 private:
    static constexpr int kMaxNumberSize = 32;

    Buffer buffer_;
};

}  // namespace log

#endif  // LOG_LOG_STREAM_H_
