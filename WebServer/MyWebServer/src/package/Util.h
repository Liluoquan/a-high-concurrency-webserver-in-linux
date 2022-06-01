// @Author: Lawson
// @Date: 2022/03/25

#pragma once
#include <cstdlib>
#include <string>
#include <sys/epoll.h>

// size_t用于数组下标和内存管理函数
// ssize_t表示读写操作的数据块的大小
ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string& inBuffer, bool& zero);
ssize_t readn(int fd, std::string& inBuffer);

ssize_t writen(int fd, void* buff, size_t n);
ssize_t writen(int fd, std::string& sbuff);

// 对open和close的封装
int Open(const char *pathname,int oflags,mode_t mode);
int Close(int srcfd);

// epoll的封装
int Epoll_create(int size);
int Epoll_create1(int flags);
int Epoll_ctl(int epfd, int op, int fd, epoll_event *event);
int Epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout);

int Eventfd(unsigned int initval, int flags);

void handle_for_sigpipe();
int setSocketNonBlocking(int fd);
void setSocketNoDelay(int fd);
void setSocketNoLinger(int fd);
void shutDownWR(int fd);
int socket_bind_listen(int port);
