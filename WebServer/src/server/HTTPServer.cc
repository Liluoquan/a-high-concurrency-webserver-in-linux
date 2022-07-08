// @Author: Lawson
// @CreateDate: 2022/04/06
// @LastModifiedDate: 2022/04/06

#include "HTTPServer.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>
#include <memory>
#include "../package/Util.h"
#include "../base/Logging.h"

using namespace std;

HTTPServer::HTTPServer(int threadNum, int port)
    : loop_(newElement<EventLoop>(), deleteElement<EventLoop>),
      acceptChannel_(newElement<Channel>(loop_), deleteElement<Channel>),
      threadNum_(threadNum),
      eventLoopThreadPool_(newElement<EventLoopThreadPool>(threadNum), deleteElement<EventLoopThreadPool>),
      started_(false),
      port_(port),
      listenFd_(socket_bind_listen(port_))
{
    if (setSocketNonBlocking(listenFd_) < 0)
    {
        perror("set socket non-blocking failed");
    }

    acceptChannel_->setFd(listenFd_);
    // handle_for_sigpipe();
}

HTTPServer::~HTTPServer() {
    Close(listenFd_);
}

void HTTPServer::start()
{
    eventLoopThreadPool_->start(); // 创建线程池，开始每个eventloop.loop()
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptChannel_->setReadHandler(bind(&HTTPServer::handleNewConn, this));

    loop_->addToPoller(acceptChannel_);
    LOG << "HTTPServer start!";
    loop_->loop();
}

void HTTPServer::handleNewConn()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                               &client_addr_len)) > 0)
    {
        // 设为非阻塞模式
        if (setSocketNonBlocking(accept_fd) < 0)
        {
            LOG << "Set non-blocking failed";
            return;
        }

        sp_EventLoop nextLoop = eventLoopThreadPool_->getNextLoop();
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
            << ntohs(client_addr.sin_port);

        // 限制服务器的最大并发连接数
        // TODO:关闭不常用的fd
        if (accept_fd >= MAXFDS)
        {
            close(accept_fd);
            continue;
        }

        // TODO:TCP包的传输问题
        // setSocketNoDelay(accept_fd);
        // setSocketNoLinger(accept_fd);

        sp_Channel connChannel(newElement<Channel>(nextLoop, accept_fd), deleteElement<Channel>);
        std::weak_ptr<Channel> wpChannel = connChannel;

        connChannel->setCloseHandler(bind(&HTTPServer::handleClose, this, wpChannel));


        // channel由HttpData持有
        // std::shared_ptr<HttpData> req_info(new HttpData(nextLoop, accept_fd));
        // req_info->getChannel()->setHolder(req_info);
        // nextLoop->queueInLoop(std::bind(&HttpData::newEvent, req_info));

        acceptChannel_->setEvents(EPOLLIN | EPOLLET);
        sp_HttpConnection connHttp(newElement<httpConnection>(connChannel), deleteElement<httpConnection>);
        HttpMap_[accept_fd] = move(connHttp);
        
        nextLoop->queueInLoop(bind(&EventLoop::addToPoller, nextLoop, move(connChannel)));
    }
    
}

void HTTPServer::handleClose(std::weak_ptr<Channel> channel) {
    sp_Channel spChannel = channel.lock();
    
    loop_->queueInLoop(bind(&HTTPServer::deleteFromMap, this, spChannel));
    spChannel->getLoop().lock()->removeFromPoller(spChannel);

    LOG << "connection closed";
}

void HTTPServer::deleteFromMap(sp_Channel requst) {
    HttpMap_.erase(requst->getFd());
}

