#ifndef EVENT_POLLER_H_
#define EVENT_POLLER_H_ 

#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "timer/timer_heap.h"
#include "utility/noncopyable.h"

//类的前置声明
namespace http {
class HttpConnection;
}  // namespace http

namespace event {
//类的前置声明
class Channel;

//IO多路复用类
class Poller : utility::NonCopyAble {
 public:
    //创建epoll内核事件表 给就绪事件，所有channel，以及对应http连接开辟内存空间
    explicit Poller(int thread_id_ = 0);
    ~Poller();
    
    //epoll_wait 等待就绪事件，然后处理就绪事件
    std::vector<std::shared_ptr<Channel>> Poll();

    // 注册新描述符(如果传入的超时时间大于0 就给此fd绑定一个定时器 以及绑定http连接)
    void EpollAdd(std::shared_ptr<Channel> channel, int timeout);
    // 修改描述符状态(如果传入的超时时间大于0 就给此fd绑定一个定时器)
    void EpollMod(std::shared_ptr<Channel> channel, int timeout);
    // 从epoll中删除描述符
    void EpollDel(std::shared_ptr<Channel> channel);

    //添加定时器
    void AddTimer(std::shared_ptr<Channel> channel, int timeout);
    //处理超时
    void HandleExpire();

    int epoll_fd() {
        return epoll_fd_;
    }

 private:
    static constexpr int kMaxFdNum = 100000;      //最大fd数量
    static constexpr int kMaxEventsNum = 4096;   //最大事件数量
    static constexpr int kEpollTimeOut = 10000;  //epoll wait的超时时间

    int epoll_fd_;                                                       //epoll的文件描述符
    std::vector<epoll_event> ready_events_;                              //就绪事件
    std::shared_ptr<Channel> ready_channels_[kMaxFdNum];                 //就绪fd的channel
    std::shared_ptr<http::HttpConnection> http_connections_[kMaxFdNum];  //http连接对象    
    timer::TimerHeap timer_heap_;                                        //定时器小顶堆

    int thread_id_;
    int connection_num_;
};

}  // namespace event

#endif  // EVENT_POLLER_H_
