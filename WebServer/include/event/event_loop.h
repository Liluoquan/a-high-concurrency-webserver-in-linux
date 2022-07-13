#ifndef EVENT_EVENT_LOOP_H_
#define EVENT_EVENT_LOOP_H_

#include <assert.h>

#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "channel.h"
#include "poller.h"
#include "thread/thread.h"
#include "locker/mutex_lock.h"
#include "utility/socket_utils.h"

namespace event {
// Reactor模式的核心 每个Reactor线程内部调用一个EventLoop
// 内部不停的进行epoll_wait调用 然后调用fd对应Channel的相应回调函数进行处理
class EventLoop {
 public:
    typedef std::function<void()> Function;

    //初始化poller, event_fd，给event_fd注册到epoll中并注册其事件处理回调
    EventLoop();
    ~EventLoop();

    //开始事件循环 调用该函数的线程必须是该EventLoop所在线程，也就是Loop函数不能跨线程调用
    void Loop();

    //停止Loop
    void StopLoop();

    //如果当前线程就是创建此EventLoop的线程 就调用callback(关闭连接 EpollDel) 否则就放入等待执行函数区
    void RunInLoop(Function&& func);
    //把此函数放入等待执行函数区 如果当前是跨线程 或者正在调用等待的函数则唤醒
    void QueueInLoop(Function&& func);

    //把fd和绑定的事件注册到epoll内核事件表
    void PollerAdd(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->EpollAdd(channel, timeout);
    }

    //在epoll内核事件表修改fd所绑定的事件
    void PollerMod(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->EpollMod(channel, timeout);
    }

    //从epoll内核事件表中删除fd及其绑定的事件
    void PollerDel(std::shared_ptr<Channel> channel) {
        poller_->EpollDel(channel);
    }

    //只关闭连接(此时还可以把缓冲区数据写完再关闭)
    void ShutDown(std::shared_ptr<Channel> channel) {
        utility::ShutDownWR(channel->fd());
    }

    bool is_in_loop_thread() const {
        return thread_id_ == current_thread::thread_id();
    }

 private:
    //创建eventfd 类似管道的 进程间通信方式
    static int CreateEventfd();

    void HandleRead();                 //eventfd的读回调函数(因为event_fd写了数据，所以触发可读事件，从event_fd读数据)
    void HandleUpdate();               //eventfd的更新事件回调函数(更新监听事件)
    void WakeUp();                     //异步唤醒SubLoop的epoll_wait(向event_fd中写入数据)
    void PerformPendingFunctions();    //执行正在等待的函数(SubLoop注册EpollAdd连接套接字以及绑定事件的函数)

 private:    
    std::shared_ptr<Poller> poller_;           //io多路复用 分发器
    int event_fd_;                             //用于异步唤醒SubLoop的Loop函数中的Poll(epoll_wait因为还没有注册fd会一直阻塞)
    std::shared_ptr<Channel> wakeup_channel_;  //用于异步唤醒的channel
    pid_t thread_id_;                          //线程id

    mutable locker::MutexLock mutex_;
    std::vector<Function> pending_functions_;  //正在等待处理的函数

    bool is_stop_;                             //是否停止事件循环
    bool is_looping_;                          //是否正在事件循环
    bool is_event_handling_;                   //是否正在处理事件
    bool is_calling_pending_functions_;        //是否正在调用等待处理的函数
};

}  // namespace event

#endif  // EVENT_EVENT_LOOP_H_