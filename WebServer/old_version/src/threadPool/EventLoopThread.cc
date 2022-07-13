// @Author: Lawson
// @Date: 2022/04/01

#include "EventLoopThread.h"
#include "../memory/MemoryPool.h"
#include <functional>
#include <memory>

EventLoopThread::EventLoopThread()
    : loop_(newElement<EventLoop>(), deleteElement<EventLoop>),
      thread_(newElement<Thread>(std::bind(&EventLoopThread::threadFunc, this), "EventLoopThread"), 
            deleteElement<Thread>)
{
}

EventLoopThread::~EventLoopThread()
{
}

sp_EventLoop EventLoopThread::getLoop()
{
    return loop_;
}

void EventLoopThread::start()
{
    thread_->start();
}

void EventLoopThread::threadFunc()
{
    loop_->loop();
}
