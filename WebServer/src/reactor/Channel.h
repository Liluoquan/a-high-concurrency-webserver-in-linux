// @Author: Lawson
// @Date: 2022/03/30

#pragma once
#include "EventLoop.h"
#include "../base/noncopyable.h"
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>

class EventLoop;
typedef std::shared_ptr<EventLoop> sp_EventLoop;

class Channel : noncopyable {
typedef std::function<void()> CallBack;
public:
    Channel(sp_EventLoop loop);
    Channel(sp_EventLoop loop, int fd);
    ~Channel();

    int getFd() { return fd_; }
    void setFd(int fd) { fd_ = fd; }
    void setRevents(__uint32_t revt) { revents_ = revt; } // called by poller
    void setEvents(__uint32_t evt) { events_ = evt; }
    __uint32_t &getEvents() { return events_; } // 此处返回的是引用，因此外部可以更改events_的值   
    bool isDeleted() { return deleted_; }
    void setDeleted(bool deleted) { deleted_ = deleted; }
    bool isFirst() { return first_; }
    void setnotFirst() { first_ = false; }
    std::weak_ptr<EventLoop> getLoop() { return loop_; }

    void setReadHandler(CallBack&& readHandler) { readHandler_ = std::move(readHandler); }
    void setWriteHandler(CallBack&& writeHandler) { writeHandler_ = std::move(writeHandler); }
    void setCloseHandler(CallBack&& closeHandler) { closeHandler_ = std::move(closeHandler); }
    void setErrorHanler(CallBack&& errorHandler) { errorHandler_ = std::move(errorHandler); }

    void handleEvents();	// 根据revents_的状态调用对应的回调函数
    void handleRead();
    void handleWrite();
    void handleError();
    void handleClose();

private:
    int fd_;
    __uint32_t events_;         // Channel关心的事件
    __uint32_t revents_;        // epoll/poll返回的活动事件
    bool deleted_;
    bool first_;
    std::weak_ptr<EventLoop> loop_;

    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack closeHandler_;
    CallBack errorHandler_;
};

typedef std::shared_ptr<Channel> sp_Channel;
