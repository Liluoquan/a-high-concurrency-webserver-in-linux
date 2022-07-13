#ifndef EVENT_EVENT_LOOP_THREAD_POOL_H_
#define EVENT_EVENT_LOOP_THREAD_POOL_H_

#include "event_loop_thread.h"

#include <functional>
#include <memory>
#include <vector>

namespace event {
//IO线程池
class EventLoopThreadPool : utility::NonCopyAble {
 public:
    EventLoopThreadPool(EventLoop* main_loop, int thread_num);
    ~EventLoopThreadPool();

    void Start();
    EventLoop* GetNextLoop();

 private:
    EventLoop* main_loop_;
    bool is_started_;
    int thread_num_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> sub_loop_threads_;
    std::vector<EventLoop*> sub_loops_;
};

}  // namespace event

#endif  // EVENT_EVENT_LOOP_THREAD_POOL_H_
