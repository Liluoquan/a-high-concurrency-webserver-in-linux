#ifndef TIMER_TIMER_HEAP_H_
#define TIMER_TIMER_HEAP_H_

#include "timer.h"

#include <memory>
#include <deque>
#include <queue>

#include "utility/noncopyable.h"

//类的前置声明
namespace http {
class HttpConnection;
}  // namespace http

namespace timer {
//定时器比较仿函数 升序
struct TimerCompare {
    bool operator()(const std::shared_ptr<Timer>& a,
                    const std::shared_ptr<Timer>& b) const {
        return a->expire_time() > b->expire_time();
    }
};

//定时器小根堆
class TimerHeap : utility::NonCopyAble {
 public:
    TimerHeap() {
    }
    ~TimerHeap() {
    }

    //添加定时器 将其添加到小根堆中
    void AddTimer(std::shared_ptr<http::HttpConnection> http_connection, int timeout);
    //处理到期事件 如果定时器被删除或者已经到期 就从小根堆中删除
    void HandleExpireEvent();

 private:
    //优先级队列 小顶堆
    std::priority_queue<std::shared_ptr<Timer>, 
                        std::deque<std::shared_ptr<Timer>>, 
                        TimerCompare> timer_heap_;
};

}  // namespace timer

#endif  // namespace TIMER_TIMER_H_