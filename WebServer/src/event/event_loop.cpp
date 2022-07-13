#include "event/event_loop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>

#include "utility/socket_utils.h"
#include "log/logging.h"

namespace event {
//线程局部变量 记录本线程持有的EventLoop的指针 
//一个线程最多持有一个EventLoop 所以创建EventLoop时检查该指针是否为空
__thread EventLoop* tls_event_loop = NULL;

//初始化poller, event_fd，给event_fd注册到epoll中并注册其事件处理回调
EventLoop::EventLoop()
    : is_looping_(false), 
      is_stop_(false), 
      is_event_handling_(false),
      is_calling_pending_functions_(false) {
    //这个eventloop是属于这个线程id的
    thread_id_ = current_thread::thread_id();
    //创建poller io复用 
    poller_ = std::make_shared<Poller>(thread_id_);
    //创建进程间通信event_fd,用于异步唤醒SubLoop的Loop函数中的Poll(epoll_wait因为还没有注册fd会一直阻塞)
    event_fd_ = CreateEventfd();
    //创建event_fd的channel 用于异步唤醒SubLoop
    wakeup_channel_ = std::make_shared<Channel>(event_fd_);

    //如果本线程已经拥有一个EventLoop 就不能再拥有了
    if (tls_event_loop) {
        LOG(WARNING) << "Another EventLoop " << tls_event_loop << " exists in this thread " << thread_id_;
    } else {
        tls_event_loop = this;
    }
    
    //给event_fd设置事件以及发生事件后调用的CallBack回调函数
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
    wakeup_channel_->set_read_handler(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->set_update_handler(std::bind(&EventLoop::HandleUpdate, this));
    //将event_fd注册到epoll内核事件表中
    poller_->EpollAdd(wakeup_channel_, 0);
}

EventLoop::~EventLoop() {
    close(event_fd_);
    tls_event_loop = NULL;
}

//开始事件循环 调用该函数的线程必须是该EventLoop所在线程 
//1. epoll_wait阻塞 等待就绪事件(没有注册其他fd时，可以通过event_fd来异步唤醒)
//2. 处理每个就绪事件
//3. 执行正在等待的函数(fd注册到epoll内核事件表)
//4. 处理超时事件 到期了就从定时器小根堆中删除
void EventLoop::Loop() {
    //判断是否在当前线程
    assert(!is_looping_);
    assert(is_in_loop_thread());
    is_looping_ = true;
    is_stop_ = false;

    while (!is_stop_) {
        //epoll_wait阻塞 等待就绪事件
        auto ready_channels = poller_->Poll();
        is_event_handling_ = true;
        //处理每个就绪事件(不同channel绑定了不同的callback)
        for (auto& channel : ready_channels) {
            channel->HandleEvents();
        }

        is_event_handling_ = false;
        //执行正在等待的函数(fd注册到epoll内核事件表)
        PerformPendingFunctions();
        //处理超时事件 到期了就从定时器小根堆中删除(定时器析构会EpollDel掉fd)
        poller_->HandleExpire();
    }

    is_looping_ = false;
}

//停止Loop, 如果是跨线程 则唤醒(向event_fd中写入数据)
void EventLoop::StopLoop() {
    is_stop_ = true;
    if (!is_in_loop_thread()) {
        WakeUp();
    }
}

//执行正在等待的函数(这里的func是SubLoop注册EpollAdd连接套接字以及绑定事件的函数)
void EventLoop::PerformPendingFunctions() {
    std::vector<Function> functions;
    is_calling_pending_functions_ = true;

    //先将正在等待执行的函数拿到本地 再执行 这样锁的时间就会小很多
    {
        locker::LockGuard lock(mutex_);
        functions.swap(pending_functions_);
    }   

    for (const auto& func : functions) {
        func();
    }
    is_calling_pending_functions_ = false;
}

// 把此函数放入等待执行函数区 如果当前是跨线程 或者正在调用等待的函数则唤醒
void EventLoop::QueueInLoop(Function&& func) {
    {
        locker::LockGuard lock(mutex_);
        pending_functions_.emplace_back(std::move(func));
    }

    //如果跨线程调用 或者当前loop正在运行等待的函数 异步唤醒epoll_wait（向event_fd中写入数据触发可读事件）
    if (!is_in_loop_thread() || is_calling_pending_functions_) {
        WakeUp();
    }
}

//如果当前线程就是创建此EventLoop的线程 就调用callback(关闭连接 EpollDel) 否则就放入等待执行函数区
void EventLoop::RunInLoop(Function&& func) {
    if (is_in_loop_thread()) {
        func();
    } else {
        QueueInLoop(std::move(func));
    }
}

//异步唤醒SubLoop的epoll_wait, 向event_fd中写入数据1
void EventLoop::WakeUp() {
    uint64_t value = 1;
    ssize_t write_size = utility::Write(event_fd_, (char*)(&value), sizeof(value));
    if (write_size != sizeof(value)) {
        LOG(WARNING) << "EventLoop::WakeUp() writes " << write_size << " bytes instead of 8";
    }
}

//异步唤醒SubLoop的epoll_wait, eventfd的读回调函数,从eventfd读出数据1
void EventLoop::HandleRead() {
    uint64_t value = 1;
    ssize_t read_size = utility::Read(event_fd_, &value, sizeof(value));
    if (read_size != sizeof(value)) {
        LOG(WARNING) << "EventLoop::HandleRead() reads " << read_size << " bytes instead of 8";
    }

    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
}

//eventfd的连接回调函数 更新监听事件events
void EventLoop::HandleUpdate() {
    PollerMod(wakeup_channel_);
}

//创建eventfd 类似管道的进程间通信方式 eventfd包含一个由内核维护的64位无符号整型计数器
//迪奥read/write来读取/改变计数器的值，从而实现进程间通信。
//用于异步唤醒SubLoop的Loop函数中的Poll(epoll_wait因为还没有注册fd会一直阻塞)
int EventLoop::CreateEventfd() {
    //设置非阻塞套接字
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        LOG(FATAL) << "Create eventfd failed";
    }

    return event_fd;
}

}  // namespace event
