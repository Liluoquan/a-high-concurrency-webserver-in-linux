#include "connection.h"

udpConnection::udpConnection(int port) : port_(port) {
    init();
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_.sin_port = htons(port_);
}

void udpConnection::init() {
    fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd_ < 0) {
        LOG << "create UDP failed";
        exit(0);
    }
    bzero(&addr_, sizeof(addr_));
}

void udpConnection::checkMsg() {
    
}

std::vector<udpConnection::udpConnectionPtr> udpPool::udpList_;
MutexLock udpPool::mutex_;
udpPool::udpPoolPtr udpPool::instance_ = nullptr;
int udpPool::index_ = -1;

udpPool::udpPoolPtr udpPool::getUdpPool() {
    {
        MutexLockGuard lock(mutex_);
        if(instance_ == nullptr)
            instance_ = udpPoolPtr(new udpPool());
    }
    return instance_;
}

udpPool::udpPool() {
    int basePort = 10000;
    for(int i = 0; i < MAX_NUM; ++i) {
        udpList_.emplace_back(new udpConnection(basePort + i));
    }
}

udpConnection::udpConnectionPtr udpPool::getNextUDP() {
    index_ = (index_ + 1) % MAX_NUM;
    return udpList_[index_];
}


tcpConnection::tcpConnection(int port) : port_(port) {
    
}

