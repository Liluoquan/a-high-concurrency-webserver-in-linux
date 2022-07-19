#include "server/web_server.h"

#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <memory>
#include <functional>

#include "utility/socket_utils.h"
// #include "log/logging.h"

namespace server
{
    
WebServer::WebServer(event::EventLoop* event_loop, int thread_num, int port)
    : event_loop_(event_loop), 
      thread_num_(thread_num),
      port_(port), 
      is_started_(false) {
    // new一个事件循环线程池 和用于接收的Channel
    event_loop_thread_pool_ = std::unique_ptr<event::EventLoopThreadPool>(new event::EventLoopThreadPool(
                                                                          event_loop_, thread_num));
    accept_channel_ = std::make_shared<event::Channel>();

    // 绑定服务器ip和端口 监听端口
    listen_fd_ = utility::SocketListen(port_); 
    accept_channel_->set_fd(listen_fd_);
    utility::HandlePipeSignal();

    // 设置NIO非阻塞套接字
    if (utility::SetSocketNonBlocking(listen_fd_) < 0) {
        perror("set socket non block failed");
        abort();
    }
}

// 初始化
void WebServer::Initialize(event::EventLoop* event_loop, int thread_num, int port) {
    event_loop_ = event_loop;
    thread_num_ = thread_num;
    port_ = port;
    is_started_ = false;

    // new一个事件循环线程池 和用于接收的Channel
    event_loop_thread_pool_ = std::unique_ptr<event::EventLoopThreadPool>(new event::EventLoopThreadPool(
                                                                          event_loop_, thread_num));
    accept_channel_ = std::make_shared<event::Channel>();

    // 绑定服务器ip和端口 监听端口
    listen_fd_ = utility::SocketListen(port_); 
    accept_channel_->set_fd(listen_fd_);
    utility::HandlePipeSignal();

    // 设置NIO非阻塞套接字
    if (utility::SetSocketNonBlocking(listen_fd_) < 0) {
        perror("set socket non block failed");
        abort();
    }
}

// 单例模式（懒汉式）
WebServer& WebServer::GetInstance() {
    static WebServer webServer_;
    return webServer_;
}

//开始
void WebServer::Start() {
    //开启event_loop线程池
    event_loop_thread_pool_->Start();
    //accept的管道
    accept_channel_->set_events(EPOLLIN | EPOLLET);
    accept_channel_->set_read_handler(std::bind(&WebServer::HandleNewConnect, this));
    accept_channel_->set_update_handler(std::bind(&WebServer::HandelCurConnect, this));

    event_loop_->PollerAdd(accept_channel_, 0);
    is_started_ = true;
}

void WebServer::HandleNewConnect() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        int connect_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_addr_len);
        event::EventLoop* event_loop = event_loop_thread_pool_->GetNextLoop();
        // LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) 
        //     << ":" << ntohs(client_addr.sin_port);
        if (connect_fd < 0) {
            // LOG << "Accept failed!";
            break;
        }
        // 限制服务器的最大并发连接数
        if (connect_fd >= MAX_FD_NUM) {
            // LOG << "Internal server busy!";
            close(connect_fd);
            break;
        }
        // 设为非阻塞模式
        if (utility::SetSocketNonBlocking(connect_fd) < 0) {
            // LOG << "set socket nonblock failed!";
            close(connect_fd);
            break;
        }
        //设置套接字
        utility::SetSocketNoDelay(connect_fd);
        // setSocketNoLinger(connect_fd);

        std::shared_ptr<http::HttpConnection> req_info(new http::HttpConnection(event_loop, connect_fd));
        req_info->connect_channel()->set_holder(req_info);

        event_loop->QueueInLoop(std::bind(&http::HttpConnection::Register, req_info));
    }

    accept_channel_->set_events(EPOLLIN | EPOLLET);
}

void WebServer::HandelCurConnect() {
    event_loop_->PollerMod(accept_channel_);
} 

} // namespace server

