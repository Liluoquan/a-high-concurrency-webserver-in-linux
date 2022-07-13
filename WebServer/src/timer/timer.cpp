#include "timer/timer.h"

#include <sys/time.h>
#include <unistd.h>

#include <memory>
#include <queue>

#include "http/http_connection.h"
#include "log/logging.h"

namespace timer {

Timer::Timer(std::shared_ptr<http::HttpConnection> http_connection, int timeout)
    : http_connection_(http_connection),
      is_deleted_(false) {
    // 以毫秒计算 到期时间 = 当前时间(秒余到10000以内换算成毫秒 + 微妙换算成毫秒) + 超时时间(毫秒)
    struct timeval now;
    gettimeofday(&now, NULL);
    expire_time_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//拷贝构造函数
Timer::Timer(Timer& timer)
    : http_connection_(timer.http_connection_), 
      expire_time_(0) {
}

//定时器析构时 (如果是因重新绑定新定时器而将此旧定时器删除的情况，http对象reset,所以不会调用Delete)
//如果是因为超时而将此定时器删除的情况 就会调用http的Delete(EpollDel, close(fd))
Timer::~Timer() {
    if (http_connection_) {
        LOG(INFO) << "Timeout, close sockfd: " << http_connection_->connect_fd();
        http_connection_->Delete();
    }
}

//更新到期时间 = 当前时间 + 超时时间
void Timer::Update(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    expire_time_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//是否到期
bool Timer::is_expired() {
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t current_time = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if (current_time >= expire_time_) {
        is_deleted_ = true;
        return true;
    }
    
    return false;
}

//释放http
void Timer::Release() {
    http_connection_.reset();
    is_deleted_ = true;
}

}  // namespace timer
