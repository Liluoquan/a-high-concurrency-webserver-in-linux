// @Author: Lawson
// @Date: 2022/04/01

#pragma once
#include "../reactor/EventLoop.h"
#include "../base/Condition.h"
#include "../base/MutexLock.h"
#include "../base/Thread.h"
#include "../base/noncopyable.h"
#include "../memory/MemoryPool.h"


class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();
    void start();
    sp_EventLoop getLoop();

private:
    void threadFunc();  //线程运行函数
    sp_EventLoop loop_;
    std::unique_ptr<Thread, decltype(deleteElement<Thread>)*> thread_;
};










