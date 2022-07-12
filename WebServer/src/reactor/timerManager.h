// @Author: Lawson
// @Date: 2022/07/12

#pragma once

#include "timer.h"
#include "../base/noncopyable.h"

#include <memory>
#include <deque>
#include <queue>

class httpConnection;

namespace timer {

// 定时器队列的比较函数，超时时间短的在前面
struct timerCompare {
    bool operator() (const std::shared_ptr<Timer>& a,
                     const std::shared_ptr<Timer>& b) const {
        return a->getExpiredTime() > b->getExpiredTime();
    }
};

// 定时器队列（小根堆）
class TimerManager : noncopyable {
public:
    TimerManager() {}
    ~TimerManager() {}

    void addTimer(std::shared_ptr<httpConnection> pHttpConnection, int timeout);
    void handleExpireEvent();

private:
    std::priority_queue<std::shared_ptr<Timer>,
                        std::deque<std::shared_ptr<Timer>>,
                        timerCompare> timerHeap_;

};


}  // namespace timer

