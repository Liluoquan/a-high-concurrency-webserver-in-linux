#ifndef EVENT_CHANNEL_H_
#define EVENT_CHANNEL_H_

#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "http/http_connection.h"

namespace event {
// Channel封装了一系列fd对应的操作 使用EventCallBack回调函数的手法
// 包括处理读 处理写 处理错误 处理连接4个回调函数
// fd一般是tcp连接connfd(套接字fd), 或者timerfd(定时器fd)，文件fd
class Channel {
 public:
    typedef std::function<void()> EventCallBack;

    Channel();
    explicit Channel(int fd);
    ~Channel();

    //IO事件的回调函数 EventLoop中调用Loop开始事件循环 会调用Poll得到就绪事件 
    //然后依次调用此函数处理就绪事件
    void HandleEvents();      
    void HandleRead();       //处理读事件的回调
    void HandleWrite();      //处理写事件的回调
    void HandleUpdate();     //处理更新事件的回调
    void HandleError();      //处理错误事件的回调

    int fd() const {
        return fd_;
    }
    
    void set_fd(int fd) {
        fd_ = fd;
    }

    //返回weak_ptr所指向的shared_ptr对象
    std::shared_ptr<http::HttpConnection> holder() {
        std::shared_ptr<http::HttpConnection> http(holder_.lock());
        return http;
    }

    void set_holder(std::shared_ptr<http::HttpConnection> holder) {
        holder_ = holder;
    }

    void set_read_handler(EventCallBack&& read_handler) {
        read_handler_ = read_handler;
    }

    void set_write_handler(EventCallBack&& write_handler) {
        write_handler_ = write_handler;
    }

    void set_update_handler(EventCallBack&& update_handler) {
        update_handler_ = update_handler;
    }

    void set_error_handler(EventCallBack&& error_handler) {
        error_handler_ = error_handler;
    }

    void set_revents(int revents) {
        revents_ = revents;
    }

    int& events() {
        return events_;
    }

    void set_events(int events) {
        events_ = events;
    }

    int last_events() const {
        return last_events_;
    }

    bool update_last_events() {
        bool is_events_changed = (last_events_ == events_);
        last_events_ = events_;
        return is_events_changed;
    }

 private:
    int fd_;            //Channel的fd
    int events_;        //Channel正在监听的事件
    int revents_;       //返回的就绪事件
    int last_events_;   //上一此事件（主要用于记录如果本次事件和上次事件一样 就没必要调用epoll_mod）

    //weak_ptr是一个观测者（不会增加或减少引用计数）,同时也没有重载->,和*等运算符 所以不能直接使用
    //可以通过lock函数得到它的shared_ptr（对象没销毁就返回，销毁了就返回空shared_ptr）
    //expired函数判断当前对象是否销毁了 
    std::weak_ptr<http::HttpConnection> holder_;  

    EventCallBack read_handler_;   
    EventCallBack write_handler_;
    EventCallBack update_handler_;
    EventCallBack error_handler_;
};

}  // namespace event

#endif  // EVENT_CHANNEL_H_