// @Author: Lawson
// @Date: 2022/03/30

#include "Channel.h"
#include "Epoll.h"
#include "EventLoop.h"
#include "../package/Util.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <queue>

using namespace std;

Channel::Channel(sp_EventLoop loop)
    : fd_(0), events_(0), deleted_(false), first_(true) , loop_(loop) {}

Channel::Channel(sp_EventLoop loop, int fd)
    : fd_(fd), events_(0), deleted_(false), first_(true) , loop_(loop) {}

Channel::~Channel() {
    // FIXME:close(fd_)?
    LOG << "close fd " << fd_;
    Close(fd_);
}

void Channel::handleRead() {
    if(readHandler_) {
        readHandler_();
    }
}

void Channel::handleWrite() {
    if(writeHandler_) {
        writeHandler_();
    }
}

void Channel::handleError() {
    if(errorHandler_) {
        errorHandler_();
    }
}

void Channel::handleClose() {
    if(closeHandler_) {
        closeHandler_();
    }
}


// 根据revents_的状态调用对应的回调函数
void Channel::handleEvents() {
    // events_ = 0;
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        // events_ = 0;
        // TODO:设置关闭回调函数
        handleClose();
        return;
    }
    if(revents_ & EPOLLERR) {
        handleError();
        // events_ = 0;
        return;
    }
    if(revents_ & (EPOLLIN /* | EPOLLPRI | EPOLLRDHUP */)) {
        handleRead();
    }
    if(revents_ & EPOLLOUT) {
        handleWrite();
    }
}






