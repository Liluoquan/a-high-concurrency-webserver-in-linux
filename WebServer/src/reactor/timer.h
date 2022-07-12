// @Author: Lawson
// @Date: 2022/07/12

#pragma once

#include "../connection/httpConnection.h"

#include <memory>

class httpConnection;

namespace timer {
    
// 定时器类
class Timer {
public:
    Timer(std::shared_ptr<httpConnection> pHttpConnection, int timeout);
    Timer(Timer& timer);
    ~Timer();

    // 更新到期时间 = 当前时间 + 超时时间
    void update(int timeout);
    // 是否到期
    bool isExpired();
    // 释放http连接
    void release();
    // 获取到期时间
    int getExpiredTime() const { return expireTime_; }
    // http连接是否已经删除
    bool isDeleted() const { return isDeleted_; }

private:
    std::shared_ptr<httpConnection> pHttpConnection_;
    size_t expireTime_;
    bool isDeleted_;
};




}  // namespace timer

