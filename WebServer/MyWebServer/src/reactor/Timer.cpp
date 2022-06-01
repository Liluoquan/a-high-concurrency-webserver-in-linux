// @Author: Lawson
// @Date: 2022/03/26

#include "Timer.h"
#include "../memory/MemoryPool.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

// 此处的timeout可以理解为请求的延迟关闭时间
TimerNode::TimerNode(sp_Channel channel, int timeout)
    : channel_(channel) {
    struct timeval now;
    gettimeofday(&now, NULL);
    //FIXME:获取当前时间的毫秒？
    expiredTime_ = 
        (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode() {

}


// 延长Timer的到时时间
void TimerNode::update(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ = 
        (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid() {
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if(temp < expiredTime_)
        return true;
    else {
        // this->setDeleted();
        return false;
    }
}

bool TimerNode::isDeleted() const{
    return channel_->isDeleted();
}


void TimerManager::addTimer(sp_Channel channel, int timeout) {
    sp_TimerNode new_node(newElement<TimerNode>(channel, timeout), deleteElement<TimerNode>);
    int fd = channel->getFd();

    if(channel->isFirst()) {
        timerHeap_.push(new_node);
        channel->setnotFirst();
    }
    timerMap_[fd] = std::move(new_node);
}

// 一个Channel对应多个TimerNode，当该TimerNode节点全部弹出时，删除该事件
void TimerManager::handleExpiredEvent() {
    while(!timerHeap_.empty()) {
        sp_TimerNode ptimer_now = timerHeap_.top();
        if(ptimer_now->isDeleted() || !ptimer_now->isValid()) {
            timerHeap_.pop();
            sp_TimerNode timer = timerMap_[ptimer_now->getChannel()->getFd()];
            if(ptimer_now == timer) {
                timerMap_.erase(ptimer_now->getChannel()->getFd());
                ptimer_now->getChannel()->handleClose();
            }
            else
                timerHeap_.push(timer);
        }
        else
            break;
    }
}



