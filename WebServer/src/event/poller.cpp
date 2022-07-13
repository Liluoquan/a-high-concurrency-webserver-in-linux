#include "event/poller.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>
#include <queue>

#include "event/channel.h"
#include "timer/timer.h"
#include "http/http_connection.h"
#include "log/logging.h"

namespace event {

//创建epoll内核事件表 给就绪事件，就绪channel，以及对应http连接开辟内存空间
Poller::Poller(int thread_id)
    : thread_id_(thread_id), 
      connection_num_(0) {
    //创建epoll内核事件表
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC); 
    assert(epoll_fd_ > 0);
    ready_events_.resize(kMaxEventsNum);
}

Poller::~Poller() {
}

// io多路复用 epoll_wait返回就绪事件数 
std::vector<std::shared_ptr<Channel>> Poller::Poll() {
    while (true) {
        //epoll_wait等待就绪事件
        int events_num = epoll_wait(epoll_fd_, &(*ready_events_.begin()), 
                                    kMaxEventsNum, kEpollTimeOut);
        if (events_num < 0) {
            LOG(ERROR) << "Epoll wait error, " << strerror(errno);
        }
        
        //遍历就绪事件
        std::vector<std::shared_ptr<Channel>> ready_channels;
        for (int i = 0; i < events_num; ++i) {
            // 获取就绪事件的描述符
            int fd = ready_events_[i].data.fd;
            int events = ready_events_[i].events;

            auto ready_channel = ready_channels_[fd];
            if (ready_channel) {
                //给channel设置就绪的事件
                ready_channel->set_revents(events);
                //重置channel监听的事件 
                ready_channel->set_events(0);
                //添加到就绪channels中
                ready_channels.push_back(ready_channel);
            } else {
                LOG(WARNING) << "Channel is invalid";
            }
        }

        //如果有就绪事件就返回
        if (ready_channels.size() > 0) {
            return ready_channels;
        }
    }
}

// 注册新描述符(如果传入的超时时间大于0 就给此fd绑定一个定时器 以及绑定http连接)
void Poller::EpollAdd(std::shared_ptr<Channel> channel, int timeout) {
    int fd = channel->fd();
    int events = channel->events();
    //如果传入的超时时间大于0 就给此fd绑定一个定时器 以及绑定http连接
    if (timeout > 0) {
        //给此fd绑定一个定时器
        AddTimer(channel, timeout);
        //绑定http连接
        http_connections_[fd] = channel->holder();
    }

    //注册要监听的事件
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    //把本次监听的事件赋值为上一次事件
    channel->update_last_events();
    //将channel添加到channel array中
    ready_channels_[fd] = channel;
    
    //注册fd以及监听的事件到epoll内核事件表
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        LOG(ERROR) << "Epoll add error, " << strerror(errno);
        ready_channels_[fd].reset();
    }
    //LOG(WARNING) << "thread id: " << thread_id_ << ", connection num: " << ++connection_num_; 
}

// 修改描述符状态(如果传入的超时时间大于0 就给此fd绑定一个定时器)
void Poller::EpollMod(std::shared_ptr<Channel> channel, int timeout) {
    int fd = channel->fd();
    int events = channel->events();
    if (timeout > 0) {
        AddTimer(channel, timeout);
    }

    //把本次监听的事件赋值为上一次事件, 
    //如果本次和上次监听事件没变 则不用调用EPOLL_MOD
    if (!channel->update_last_events()) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = events;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0) {
            LOG(ERROR) << "Epoll mod error, " << strerror(errno);
            ready_channels_[fd].reset();
        }
    }
}

// 从epoll中删除描述符 并删除其channel和http连接
void Poller::EpollDel(std::shared_ptr<Channel> channel) {
    int fd = channel->fd();
    int events = channel->last_events();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        LOG(ERROR) << "Epoll del error, " << strerror(errno);
    }
    ready_channels_[fd].reset();
    http_connections_[fd].reset();
    //LOG(WARNING) << "thread id: " << thread_id_ << ", connection num: " << --connection_num_; 
}

//添加定时器
void Poller::AddTimer(std::shared_ptr<Channel> channel, int timeout) {
    //如果设置了http连接对象 才会添加定时器（比如监听套接字channel就是没有设置http连接对象的）
    auto http_connection = channel->holder();
    if (http_connection) {
        timer_heap_.AddTimer(http_connection, timeout);
    } else {
        LOG(WARNING) << "Timer add failed";
    }
}

//处理超时,超时就删除定时器(定时器析构会EpollDel注销fd)，然后释放http连接，close(fd)
void Poller::HandleExpire() {
    timer_heap_.HandleExpireEvent();
}

}  // namespace event
