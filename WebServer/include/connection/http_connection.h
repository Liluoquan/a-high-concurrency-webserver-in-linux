#ifndef HTTP_HTTP_CONNECTION_H_
#define HTTP_HTTP_CONNECTION_H_

#include "http_type.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <functional>
#include <map>

#include "timer/timer.h"
#include "pagecache/lfu_cache.h"

//类的前置声明
namespace event {
class Channel;
class EventLoop;
}  // namespace event

namespace http {
// std::enable_shared_from_this 当类http被std::shared_ptr管理，
// 且在类http的成员函数里需要把当前类对象作为参数传给其他函数时，就需要传递一个指向自身的std::shared_ptr
class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
 public:
    HttpConnection(event::EventLoop* event_loop, int connect_fd);
    ~HttpConnection();

    void Register();     //给fd注册默认事件(可读 | ET触发 | ONESHOT即只执行一次，下次epoll_wait不会触发了)
    void Delete();       //从epoll事件表中删除fd(绑定的定时器被删除时 会调用此函数),然后http连接释放，会close掉fd
    void Reset();        //重置HTTP状态
    void ResetTimer();   //重新加入定时器

    void set_timer(std::shared_ptr<timer::Timer> timer) {
        timer_ = timer;
    }
    
    std::shared_ptr<event::Channel> connect_channel() {
        return connect_channel_;
    }

    int connect_fd() const {
        return connect_fd_;
    }

 private:
    void HandleRead();      //处理读回调    读请求报文数据到read_buffer 解析请求报文 构建响应报文并写入write_buffer
    void HandleWrite();     //处理写回调    向客户端发送write_buffer中的响应报文数据
    void HandleUpdate();    //处理更新事件回调 
    void ReturnErrorMessage(int fd, int error_code, std::string error_message); //构建错误信息的响应（返回错误信息）

    LineState ParseRequestLine();        //解析uri（请求行）
    HeaderState ParseRequestHeader();    //解析请求头, post方法就再解析请求体
    ResponseState BuildResponse();       //构建响应报文并写入write_buffer

 private:
    static constexpr int kDefaultEvent = EPOLLIN | EPOLLET | EPOLLONESHOT; //默认事件
    static constexpr int kDefaultTimeOut = 2 * 1000;                //默认超时时间 ms
    static constexpr int kDefaultKeepAliveTime = 5 * 60 * 1000;     //默认长连接保持时间 ms

    int connect_fd_;                                    //连接套接字fd
    event::EventLoop* event_loop_;                      //事件循环
    std::shared_ptr<event::Channel> connect_channel_;   //连接套接字的管道
    std::weak_ptr<timer::Timer> timer_;                 //定时器
    std::string read_buffer_;                           //读缓冲区
    std::string write_buffer_;                          //写缓冲区

    ProcessState process_state_;                 //处理状态
    ConnectionState connection_state_;           //服务器和客户端的连接状态
    ParseState parse_state_;                     //解析状态 
    RequestMethod method_;                       //http请求方法
    HttpVersion version_;                        //http版本
    
    std::string file_name_;                      //请求文件名
    std::string resource_dir_;                   //资源路径
    std::map<std::string, std::string> request_headers_; //请求头字段
    bool is_error_;                              //是否发生错误
    bool is_keep_alive_;                         //是否长连接
};

}  // namespace http

#endif  // HTTP_HTTP_CONNECTION_H_
