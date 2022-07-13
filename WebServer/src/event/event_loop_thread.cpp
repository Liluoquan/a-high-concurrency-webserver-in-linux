#include "event/event_loop_thread.h"

namespace event {

//IO线程，绑定线程函数，也就是Loop
EventLoopThread::EventLoopThread()
    : sub_loop_(NULL), 
      is_exiting_(false),
      mutex_(),
      condition_(mutex_),
      thread_(std::bind(&EventLoopThread::Worker, this), "EventLoopThread") {
}

EventLoopThread::~EventLoopThread() {
    //停止loop后 线程join等待停止loop运行完
    is_exiting_ = true;
    if (sub_loop_ != NULL) {
        sub_loop_->StopLoop();
        thread_.Join();
    }
}

//执行线程函数, 在线程函数还没运行起来的时候 一直阻塞 运行起来了才唤醒返回该event_loop
EventLoop* EventLoopThread::StartLoop() {
    assert(!thread_.is_started());
    //执行线程函数
    thread_.Start();
    {
        locker::LockGuard lock(mutex_);
        // 一直阻塞等到线程函数运行起来 再唤醒返回
        while (sub_loop_ == NULL) {
            condition_.wait();
        }
    }

    return sub_loop_;
}

//线程函数(内部先唤醒StartLoop, 然后调用Loop事件循环)
void EventLoopThread::Worker() {
    EventLoop event_loop;
    {
        locker::LockGuard lock(mutex_);
        sub_loop_ = &event_loop;
        //已经运行线程函数了 唤醒StartLoop返回此event_loop对象 
        condition_.notify();
    }

    //Loop事件循环 
    event_loop.Loop();
    sub_loop_ = NULL;
}

}  // namespace event
