// @Author: Lawson
// @Date: 2022/03/26

#pragma once
#include <unistd.h>
#include <deque>
#include <memory>
#include <queue>
#include <unordered_map>

#include "Channel.h"
#include "../base/MutexLock.h"
#include "../base/noncopyable.h"

class Channel;
typedef std::shared_ptr<Channel> sp_Channel;

// TimerNode用于处理HTTP请求
class TimerNode {
public:
    TimerNode(sp_Channel channel, int timeout);
    ~TimerNode();
    // TimerNode(TimerNode& tn);
    void update(int timeout);
    bool isValid();
    // void setDeleted() { deleted_ = true; }
    bool isDeleted() const;
    size_t getExpTime() const { return expiredTime_; }
    sp_Channel getChannel() const { return channel_; }

private:
    // bool deleted_;
    sp_Channel channel_;
    size_t expiredTime_; // 定时器过期时间
};

typedef std::shared_ptr<TimerNode> sp_TimerNode;

struct TimerCmp {
    bool operator() (sp_TimerNode& a, sp_TimerNode& b) const {
        return a->getExpTime() > b->getExpTime();
    }
};

// TimerManager用于管理TimerNode
class TimerManager {
public:
    void addTimer(sp_Channel channel, int timeout);
    void handleExpiredEvent();
    
private:
    std::priority_queue<sp_TimerNode, std::vector<sp_TimerNode>, TimerCmp>
        timerHeap_;  // 小根堆
    std::unordered_map<int, sp_TimerNode> timerMap_;
};

typedef std::shared_ptr<TimerManager> sp_TimerManager;











