// @Author: Lawson
// @Date: 2022/07/12

#include "timer.h"
#include "../base/Logging.h"

#include <sys/time.h>
#include <unistd.h>

#include <memory>
#include <queue>

namespace timer {

Timer::Timer(std::shared_ptr<httpConnection> pHttpConnection, int timeout)
    : pHttpConnection_(pHttpConnection),
      isDeleted_(false) {
    struct timeval now;
    gettimeofday(&now, NULL);
    // 以毫秒计算
    // 到期时间 = 当前时间(秒余到10000以内换算成毫秒 + 微妙换算成毫秒) + 超时时间(毫秒)
    expireTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

Timer::Timer(Timer& timer) : pHttpConnection_(timer.pHttpConnection_), expireTime_(0) {}

// Timer析构时 (如果是因重新绑定新定时器而将此旧定时器删除的情况，http对象reset,所以不会调用Delete)
// 如果是因为超时而将此定时器删除的情况 就会调用http的Delete(EpollDel, close(fd))
Timer::~Timer() {
    if (pHttpConnection_) {
        LOG << "Timeout, close sockfd: " << pHttpConnection_->getFd();
        pHttpConnection_->release();
    }
}

void Timer::update(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    expireTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool Timer::isExpired() {
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t curTime = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if (curTime >= expireTime_) {
        isDeleted_ = true;
        return true;
    }

    return false;
}

void Timer::release() {
    pHttpConnection_->release();
    isDeleted_ = true;
}


}  // namespace timer