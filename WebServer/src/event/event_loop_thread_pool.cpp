#include "event/event_loop_thread_pool.h"

#include <memory>
#include <functional>
#include <vector>

#include "log/logging.h"

namespace event {
//IO线程池
EventLoopThreadPool::EventLoopThreadPool(EventLoop* main_loop, int thread_num)
    : main_loop_(main_loop), 
      thread_num_(thread_num), 
      is_started_(false), 
      next_(0) {
    if (thread_num_ <= 0) {
        LOG(FATAL) << "thread num <= 0";
    }
    sub_loop_threads_.reserve(thread_num_);
    sub_loops_.reserve(thread_num_);
}

EventLoopThreadPool::~EventLoopThreadPool() {
    LOG(DEBUG) << "~EventLoopThreadPool()";
}

//主线程（主Loop对象）创建event_loop线程池
void EventLoopThreadPool::Start() {
    assert(main_loop_->is_in_loop_thread());
    is_started_ = true;

    //创建event_loop_thread_pool,并将开始Loop事件循环的EventLoop对象存入array中
    for (int i = 0; i < thread_num_; ++i) {
        auto event_loop_thread = std::make_shared<EventLoopThread>();
        sub_loop_threads_.push_back(event_loop_thread);
        sub_loops_.push_back(event_loop_thread->StartLoop());
    }
}

//从已经开始Loop事件循环的EventLoop对象列表中 返回一个EventLoop对象 如果此时还没有 就返回主loop
EventLoop* EventLoopThreadPool::GetNextLoop() {
    //调用这个函数的必须是主loop线程
    assert(main_loop_->is_in_loop_thread());
    assert(is_started_);

    // 如果此时还没有开始Loop的EventLoop对象 就返回主loop
    auto event_loop = main_loop_;
    if (!sub_loops_.empty()) {
        event_loop = sub_loops_[next_];
        next_ = (next_ + 1) % thread_num_;
    }
    
    return event_loop;
}

}  // namespace event
