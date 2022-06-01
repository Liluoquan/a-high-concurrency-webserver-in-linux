// @Author: Lawson
// @Date: 2022/03/28

#include "Epoll.h"
#include "../package/Util.h"
#include "../base/Logging.h"
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>      // 定义数据结构sockaddr_in
#include <string.h>
#include <sys/epoll.h> 
#include <deque>
#include <queue>

#include <arpa/inet.h>       // 提供IP地址转换函数
#include <iostream>
using namespace std;

const int EVENTSNUM = 4096; // 最大活动事件数目,TODO:动态扩展,类似vector
const int EPOLLWAIT_TIME = -1; // ms，-1：epoll_wait()无限期阻塞
const int MAXFDS = 10000;    //最大描述符数目

typedef shared_ptr<Channel> sp_Channel;


// Epoll类管理一个epoll对象
Epoll::Epoll() : epollFd_(Epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
    assert(epollFd_ > 0);
}

Epoll::~Epoll() {
    Close(epollFd_);
}

// 注册新描述符
void Epoll::epoll_add(const sp_Channel& request) {
    int fd = request->getFd();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    Epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);

    channelMap_[fd] = move(request);
    // LOG << "add to poller";
};

// 修改描述符状态
void Epoll::epoll_mod(const sp_Channel& request) {
    int fd = request->getFd();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    Epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event);
}

// 从epoll中删除描述符
void Epoll::epoll_del(const sp_Channel& request) {
    int fd = request->getFd();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    // FIXME:貌似新版本可以不传入*event
    Epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event);

    channelMap_.erase(fd);
}

void Epoll::poll(std::vector<sp_Channel>& req) {
    int event_count = 
        Epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
	for(int i = 0; i < event_count; ++i) {
		int fd = events_[i].data.fd;
		sp_Channel temp = channelMap_[fd];
		temp->setRevents(events_[i].events);
		req.emplace_back(std::move(temp));
	}
    // LOG << "Epoll finished";
}

// // 返回活跃Channel
// std::vector<sp_Channel> Epoll::poll() {
//     while(true) {
//         int event_count = 
//             Epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
//         std::vector<sp_Channel> activeChannels = getEventsRequest(event_count);
//         if(activeChannels.size() > 0) return activeChannels;
//     }
// }

// // 根据活动事件将活动频道放入activeChannels
// std::vector<sp_Channel> Epoll::getEventsRequest(int events_num) {
//     std::vector<sp_Channel> activeChannels;
//     for(int i = 0; i < events_num; ++i) {
//         // 获取有事件产生的描述符
//         int fd = events_[i].data.fd;
//         sp_Channel activeChannel = channelMap_[fd];

//         if(activeChannel) {
//             activeChannel->setRevents(events_[i].events);
//             activeChannels.emplace_back(std::move(activeChannel));
//         }
//         else {
//             LOG << "activeChannels is invalid";
//         }
//     }

//     return activeChannels;
// }




