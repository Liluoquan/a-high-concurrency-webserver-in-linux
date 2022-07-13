#ifndef EVENT_EVENT_LOOP_THREAD_H_
#define EVENT_EVENT_LOOP_THREAD_H_

#include "event_loop.h"

namespace event {
//IO线程
class EventLoopThread : utility::NonCopyAble {
 public:
    EventLoopThread();
    ~EventLoopThread();
     
    //执行线程函数, 在线程函数还没运行起来的时候 一直阻塞 运行起来了才唤醒返回该event_loop
    EventLoop* StartLoop();

 private:
    //线程函数(就是EventLoop的Loop函数)
    void Worker();
   
 private:
    EventLoop* sub_loop_;    //子loop
    thread::Thread thread_;  //线程对象
    mutable locker::MutexLock mutex_;
    locker::ConditionVariable condition_;
    bool is_exiting_;       
};

}  // namespace event

#endif  // EVENT_EVENT_LOOP_THREAD_H_
