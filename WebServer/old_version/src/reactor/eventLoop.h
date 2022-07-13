// @Author: Lawson
// @Date: 2022/07/13

#pragma once

#include "Channel.h"
#include "Epoll.h"
#include "../base/MutexLock.h"

#include <iostream>
#include <memory>
#include <vector>

namespace event {
    
// Reactor模式的核心 每个Reactor线程内部调用一个EventLoop
// 内部不停的进行epoll_wait调用 然后调用fd对应Channel的相应回调函数进行处理

class EventLoop {
    using Functor = std::function<void()>;
public:
    EventLoop();
    ~EventLoop();

    void loop();                                // 开始事件循环 调用该函数的线程必须是该EventLoop所在线程，也就是Loop函数不能跨线程调用
    void stopLoop();                            // 停止Loop
    void runInLoop(Functor&& func);             // 如果当前线程就是创建此EventLoop的线程 就调用callback(关闭连接 EpollDel) 否则就放入等待执行函数区
    void queueInLoop(Functor&& func);           // 把此函数放入等待执行函数区 如果当前是跨线程 或者正在调用等待的函数则唤醒

    // 把fd和绑定的事件注册到epoll内核事件表
    void addToPoller(std::shared_ptr<Channel> channel, int timeout = 0);
    // 在epoll内核事件表修改fd所绑定的事件
    void addToPoller(std::shared_ptr<Channel> channel, int timeout = 0);
    // 从epoll内核事件表中删除fd及其绑定的事件
    void removeFromPoller(std::shared_ptr<Channel> channel, int timeout = 0);

private:
    static int creatEventFd();                  // 创建eventfd 类似管道的 进程间通信方式

    void handleRead();                          // eventfd的读回调函数(因为event_fd写了数据，所以触发可读事件，从event_fd读数据)
    void handleUpdate();                        // eventfd的更新事件回调函数(更新监听事件)
    void wakeup();                              // 异步唤醒SubLoop的epoll_wait(向event_fd中写入数据)
    void doPendingFunctors();                   // 执行正在等待的函数(SubLoop注册EpollAdd连接套接字以及绑定事件的函数)

private:
    std::shared_ptr<Epoll> poller_;             // io多路复用 分发器
    int eventFd_;                               // 用于异步唤醒SubLoop的Loop函数中的Poll(epoll_wait因为还没有注册fd会一直阻塞)
    std::shared_ptr<Channel> wakeupChannel_;    // 用于异步唤醒的 channel
    pid_t threadId;                             // 线程id

    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;      // 正在等待处理的函数
    
    bool isStop_;                               // 是否停止事件循环
    bool isLooping_;                            // 是否正在事件循环
    bool isEventHandling_;                      // 是否正在处理事件
    bool isCallingPendingFunctor_;              // 是否正在调用等待处理的函数

};


} // namespace event


