// @Author: Lawson
// @CreateDate: 2022/05/05
// @LastModifiedDate: 2022/05/05

#pragma once

#include <memory>
#include "../reactor/EventLoop.h"
#include "../threadPool/EventLoopThreadPool.h"
#include "../memory/MemoryPool.h"
#include "../manager/userManager.h"
// #include "../manager/fileManager.h"
#include "../manager/fileOperator.h"
#include "../connection/ftpConnection.h"

class FTPServer {
    using sp_ftpConnection = std::shared_ptr<ftpConnection>;
public:
    FTPServer(int threadNum, int port, string dir, unsigned short commandOffset);
    ~FTPServer();
    void start();

private:
    sp_EventLoop loop_;  //accept线程
    sp_Channel acceptChannel_;
    int threadNum_;
    std::unique_ptr<EventLoopThreadPool, decltype(deleteElement<EventLoopThreadPool>)*> eventLoopThreadPool_;
    bool started_;
    int port_;
    int listenFd_;
    static const int MAXFDS = 100000;

    std::string dir_;
    unsigned short commandOffset_;
    unsigned int connId_;

    // static userManager::userManagerPtr userManager_;
    // static std::string userInfoFileName_;
    userManager::userManagerPtr userManager_;
    std::unordered_map<int, sp_ftpConnection> ftpMap_;
    // fileManager::fileManagerPtr fileManager_;

    void handleNewConn();
    void handleClose(std::weak_ptr<Channel> channel);
    void deleteFromMap(sp_Channel ftpChannel);
};


