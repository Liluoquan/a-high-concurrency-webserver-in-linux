// @Author: Lawson
// @CreatedDate: 2022/05/11
// @ModifiedDate: 2022/05/11

#pragma once

#include <bits/types/struct_timeval.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <sys/select.h>
#include <sys/time.h>
#include <memory>
#include <vector>

#include "../base/Logging.h"
#include "../base/MutexLock.h"

class udpConnection {
public:
    using udpConnectionPtr = std::shared_ptr<udpConnection>;
    udpConnection(int port);
    ~udpConnection() { close(fd_); }
    int getFd() { return fd_; }
    int getPort() {return port_; }
    void setAddr(sockaddr_in addr) { addr_ = std::move(addr); }

    void checkMsg();

private:
    int fd_;
    int port_;
    struct sockaddr_in addr_;
    void init();
};

class udpPool {
public:
    using udpPoolPtr = std::shared_ptr<udpPool>;

    udpPool();
    ~udpPool();

    static udpPoolPtr getUdpPool();
    static udpConnection::udpConnectionPtr getNextUDP();

private:
    static int index_;
    static std::vector<udpConnection::udpConnectionPtr> udpList_;
    static MutexLock mutex_;
    static udpPoolPtr instance_;
    static const int MAX_NUM = 32;
    static void init();
};

class tcpConnection {
public:
    using tcpConnectionPtr = std::shared_ptr<tcpConnection>;
    tcpConnection(int port);
    ~tcpConnection() { close(fd_); }
    int getFd() { return fd_; }
    int getPort() {return port_; }
    void setAddr(sockaddr_in addr) { addr_ = std::move(addr); }

    void checkMsg();

private:
    int fd_;
    int port_;
    struct sockaddr_in addr_;
    void init();
};

class tcpPool {
public:
    using tcpPoolPtr = std::shared_ptr<tcpPool>;

    tcpPool();
    ~tcpPool();

    static tcpPoolPtr getUdpPool();
    static tcpConnection::tcpConnectionPtr getNextUDP();

private:
    static int index_;
    static std::vector<tcpConnection::tcpConnectionPtr> tcpList_;
    static MutexLock mutex_;
    static tcpPoolPtr instance_;
    static const int MAX_NUM = 32;
    static void init();
};

