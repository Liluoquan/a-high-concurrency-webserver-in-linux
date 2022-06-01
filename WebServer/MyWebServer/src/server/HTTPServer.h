// @Author: Lawson
// @CreateDate: 2022/04/06
// @LastModifiedDate: 2022/04/06

#pragma once
#include <memory>
#include <unordered_map>
#include "../reactor/EventLoop.h"
#include "../threadPool/EventLoopThreadPool.h"
#include "../memory/MemoryPool.h"
#include "../connection/httpConnection.h"

typedef std::shared_ptr<httpConnection> sp_HttpConnection;

class HTTPServer {
public:
    HTTPServer(int threadNum, int port);
    ~HTTPServer();
    void start();
    
private:
    sp_EventLoop loop_; // accept线程
    sp_Channel acceptChannel_;
    int threadNum_;
    std::unique_ptr<EventLoopThreadPool, decltype(deleteElement<EventLoopThreadPool>)*> eventLoopThreadPool_;
    bool started_;
    int port_;
    int listenFd_;
    static const int MAXFDS = 100000;
    std::unordered_map<int, sp_HttpConnection> HttpMap_;

    void handleNewConn();
    // void handleThisConn() { loop_->updatePoller(acceptChannel_); }
    void handleClose(std::weak_ptr<Channel> channel);
    void deleteFromMap(sp_Channel channel);
};