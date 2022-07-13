// Author: Lawson
// Date: 2022/04/01

#include "EventLoopThreadPool.h"
#include "../memory/MemoryPool.h"

EventLoopThreadPool::EventLoopThreadPool(int numThreads)
    : numThreads_(numThreads),
      index_(0) {
    threads_.reserve(numThreads);
    for(int i = 0; i < numThreads_; ++i) {
        std::shared_ptr<EventLoopThread> t(newElement<EventLoopThread>(), deleteElement<EventLoopThread>);
        threads_.emplace_back(t);
    }
}

void EventLoopThreadPool::start() {
    for(auto& thread : threads_) {
        thread->start();
    }
}

sp_EventLoop EventLoopThreadPool::getNextLoop() {
    index_ = (index_ + 1) % numThreads_;
    return threads_[index_]->getLoop();
}






