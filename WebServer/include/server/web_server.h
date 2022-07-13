#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#include <memory>

#include "event/channel.h"
#include "event/event_loop.h"
#include "event/event_loop_thread_pool.h"

namespace server
{
    
class WebServer {
 public:
    WebServer(event::EventLoop* event_loop, int thread_num, int port);
    ~WebServer() {}

    event::EventLoop* event_loop() {
        return event_loop_;
    }

    void Start();

    void HandleNewConnect();

    void HandelCurConnect();

 private:
    static const int MAX_FD_NUM = 100000;

    event::EventLoop* event_loop_;
    std::unique_ptr<event::EventLoopThreadPool> event_loop_thread_pool_;
    std::shared_ptr<event::Channel> accept_channel_;

    int port_;
    int listen_fd_;
    int thread_num_;
    bool is_started_;
};

} // namespace server




#endif