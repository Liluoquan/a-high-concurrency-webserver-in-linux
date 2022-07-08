// @Author: Lawson
// @Date: 2022/03/28

#pragma once
#include "Channel.h"
#include "../connection/httpConnection.h"
#include "Timer.h"
#include <sys/epoll.h>
#include <memory>
#include <unordered_map>
#include <vector>

class Channel;
typedef std::shared_ptr<Channel> sp_Channel;

class Epoll {
public:
    Epoll();
    ~Epoll();
    void epoll_add(const sp_Channel& request);
    void epoll_mod(const sp_Channel& request);
    void epoll_del(const sp_Channel& request);
    void poll(std::vector<sp_Channel>& req);
    // std::vector<sp_Channel> poll();
    // std::vector<sp_Channel> getEventsRequest(int events_num);
    // void add_timer(std::shared_ptr<Channel> request_data, int timeout);
    // int getEpollFd() { return epollFd_; }
    // void handleExpired();

private:
    // static const int MAXFDS = 100000;
    int epollFd_;
    std::vector<epoll_event> events_;           // epoll_wait()返回的活动事件都放在这个数组里
    std::unordered_map<int, sp_Channel> channelMap_;
};

typedef std::shared_ptr<Epoll> sp_Epoll;











