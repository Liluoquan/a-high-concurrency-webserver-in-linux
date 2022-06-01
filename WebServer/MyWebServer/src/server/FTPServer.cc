// @Author: Lawson
// @CreateDate: 2022/05/05
// @LastModifiedDate: 2022/05/05

#include "FTPServer.h"
#include "../package/Util.h"
#include "../base/Logging.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>

#define DEBUG
#define USERINFOFILENAME "user.txt"

using namespace std;

FTPServer::FTPServer(int threadNum, int port, std::string dir, unsigned short commandOffset) 
    : loop_(newElement<EventLoop>(), deleteElement<EventLoop>),
      acceptChannel_(newElement<Channel>(loop_), deleteElement<Channel>),
      threadNum_(threadNum),
      eventLoopThreadPool_(newElement<EventLoopThreadPool>(threadNum), deleteElement<EventLoopThreadPool>),
      started_(false),
      port_(port),
      listenFd_(socket_bind_listen(port)),
      dir_(dir), commandOffset_(commandOffset), connId_(0),
      userManager_(userManager::getUserManger(USERINFOFILENAME))
       {

    if(setSocketNonBlocking(listenFd_) < 0) {
        LOG << "set socket non-blocking failed";
        exit(EXIT_SUCCESS);
    }

    acceptChannel_->setFd(listenFd_);
    // std::cout << "dir_ = " << dir_ << std::endl;
}

FTPServer::~FTPServer() {
    Close(listenFd_);
}

void FTPServer::start() {
    eventLoopThreadPool_->start();
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptChannel_->setReadHandler(bind(&FTPServer::handleNewConn, this));

    loop_->addToPoller(acceptChannel_);
    LOG << "Server start!";
    loop_->loop();
}

void FTPServer::handleNewConn() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, 
                              &client_addr_len)) > 0) {
    
        // 设为非阻塞模式
        if (setSocketNonBlocking(accept_fd) < 0){
            LOG << "Set non-blocking failed";
            return;
        }


        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
            << ntohs(client_addr.sin_port);
        
        if (accept_fd >= MAXFDS) {
            LOG << "to many fds";
            close(accept_fd);
            continue;
        }

        // sp_EventLoop nextLoop = eventLoopThreadPool_->getNextLoop();
        // sp_Channel loginChannel(newElement<Channel>(nextLoop, accept_fd), deleteElement<Channel>);
        // std::weak_ptr<Channel> wpChannel = loginChannel;
        // loginChannel->setCloseHandler(bind(&FTPServer::handleClose, this, wpChannel));
        // loginChannel->setReadHandler(bind(&FTPServer::login, this, wpChannel));
        // loginChannel->setEvents(EPOLLIN | EPOLLET);
        // nextLoop->queueInLoop(std::bind(&EventLoop::addToPoller, nextLoop, move(loginChannel)));
        sp_EventLoop nextLoop = eventLoopThreadPool_->getNextLoop();
        sp_Channel ftpChannel(newElement<Channel>(nextLoop, accept_fd), deleteElement<Channel>);
        std::weak_ptr<Channel> wpChannel(ftpChannel);
        ftpChannel->setCloseHandler(std::bind(&FTPServer::handleClose, this, wpChannel));

        sp_ftpConnection connFtp(newElement<ftpConnection>(ftpChannel, connId_, dir_, commandOffset_), deleteElement<ftpConnection>);
        ftpMap_[accept_fd] = std::move(connFtp);

        nextLoop->queueInLoop(std::bind(&EventLoop::addToPoller, nextLoop, std::move(ftpChannel)));
    }
}

void FTPServer::handleClose(std::weak_ptr<Channel> channel) {
    sp_Channel ftpChannel = channel.lock();

    loop_->queueInLoop(bind(&FTPServer::deleteFromMap, this, ftpChannel));
    ftpChannel->getLoop().lock()->removeFromPoller(ftpChannel);

    LOG << "connection closed";
}

void FTPServer::deleteFromMap(sp_Channel ftpChannel) {
    ftpMap_.erase(ftpChannel->getFd());
}







