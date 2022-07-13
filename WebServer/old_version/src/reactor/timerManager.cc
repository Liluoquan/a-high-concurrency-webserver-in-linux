// @Author: Lawson
// @Date: 2022/07/12

#include "timerManager.h"
#include "../connection/httpConnection.h"

#include <memory>

namespace timer {

void TimerManager::addTimer(std::shared_ptr<httpConnection> pHttpConnection, int timeout) {
    // new 一个定时器，加到定时器队列中
    auto timer = std::make_shared<Timer>(pHttpConnection, timeout);
    timerHeap_.emplace(timer);
    pHttpConnection->setTimer(timer);
}

// 处理到期事件 如果定时器被删除或者已经到期 就从定时器队列中删除
// 定时器析构的时候 会调用http->close 会关闭http连接，EpollDel绑定的connect_fd
void TimerManager::handleExpireEvent() {
    while (!timerHeap_.empty()) {
        auto timer = timerHeap_.top();
        // 如果是被删除了(惰性删除 已经到期了但是还没被访问所以没被删) 或者到期了都会pop定时器
        if (timer->isDeleted() || timer->isExpired()) {
            timerHeap_.pop();
        }
        else {
            break;
        }
    }
}


}  // namespace timer






