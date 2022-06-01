// @Author: Lawson
// @Date: 2022/04/01

#pragma once
#include <vector>
#include <memory>
#include "EventLoopThread.h"
#include "../base/Logging.h"
#include "../base/noncopyable.h"

// 线程池的本质是生产者消费者模型
class EventLoopThreadPool {
public:
    EventLoopThreadPool(int numThreads);
    ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; }

    void start();
    sp_EventLoop getNextLoop();
    
private:
    int numThreads_;
    int index_;
    std::vector<std::shared_ptr<EventLoopThread>> threads_;
};







