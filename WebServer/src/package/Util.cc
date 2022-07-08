// @Author: Lawson
// @Date: 2022/03/25

#include "Util.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/eventfd.h>

const int MAX_BUFF = 4096;

// 封装read，保证读出n个字节到buff
ssize_t readn(int fd, void* buff, size_t n) {
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char* ptr = static_cast<char*>(buff);

    while(nleft > 0) {
        if((nread = read(fd, ptr, nleft)) < 0) {
            if(errno == EINTR) //syscall被中断
                nread = 0;
            else if(errno == EAGAIN) //资源不足or不满足条件
                //FIXME:continue?
                return readSum;
            else
                return -1;
        }
        else if(nread == 0)
            break;

        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }

    return readSum;
}


// 从fd中分块读所有内容到inBuffer（每次最多MAX_BUFF个字节），读完置zero为true
// FIXME:没有处理buff溢出的情况
ssize_t readn(int fd, std::string& inBuffer, bool& zero) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while(true) {
        char buff[MAX_BUFF];
        if((nread = read(fd, buff, MAX_BUFF)) < 0) {
            if(errno == EINTR)
                continue;
            else if(errno == EAGAIN)
                //FIXME:continue?
                return readSum;
            else {
                perror("read error");
                return -1;
            }
        }
        else if(nread == 0) {
            // printf("redsum = %d\n", readSum);
            zero = true;
            break;
        }

        // printf("before inBuffer.size() = %d\n", inBuffer.size());
        // printf("nread = %d\n", nread);
        readSum += nread;
        // buff += nread;
        inBuffer += std::string(buff, buff + nread);
        // printf("after inBuffer.size() = %d\n", inBuffer.size());
    }

    return readSum;
}


// 同上
ssize_t readn(int fd, std::string& inBuffer) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while(true) {
        char buff[MAX_BUFF];
        if((nread = read(fd, buff, MAX_BUFF)) < 0) {
            if(errno == EINTR)
                continue;
            else if(errno == EAGAIN)
                //FIXME:continue?
                return readSum;
            else {
                perror("read error");
                return -1; 
            }
        }
        else if(nread == 0) {
            // printf("readSum = %d\n", readSum);
            break;
        }
        // printf("before inBuffer.size() = %d\n", inBuffer.size());
        // printf("nread = %d\n", nread);
        readSum += nread;
        // buff += nread;
        inBuffer += std::string(buff, buff + nread);
        // printf("before inBuffer.size() = %d\n", inBuffer.size());
    }

    return readSum;
}

// 将buff的内容往fd写入n字节
ssize_t writen(int fd, void* buff, size_t n) {
    size_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writenSum = 0;
    char* ptr = static_cast<char*>(buff);
    while(nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if(nwritten < 0) {
                if(errno == EINTR) {
                    nwritten = 0;
                    continue;
                }
                else if(errno == EAGAIN)
                    //FIXME:continue?与触发方式相关
                    return writenSum;
                else
                    return -1;
            }
        } 
        writenSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return writenSum;
}

// 将sbuff的所有内容写入fd
ssize_t writen(int fd, std::string& sbuff) {
    size_t nleft = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writenSum = 0;
    const char* ptr = sbuff.c_str();
    while(nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if(nwritten < 0) {
                if(errno == EINTR) {
                    nwritten = 0;
                    continue;
                }
                else if(errno == EAGAIN)
                    return writenSum;
                else
                    return -1;
            }
        }
        writenSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    if(writenSum == static_cast<int>(sbuff.size()))
        sbuff.clear();
    else
        sbuff =  sbuff.substr(writenSum);
    return writenSum;
}

int Open(const char *pathname,int oflags,mode_t mode) {
	int n;
	if((n = open(pathname, oflags, mode)) < 0)
		fprintf(stderr, "open file failed\n");
	return n;
}

int Close(int srcfd) {
	int n;
	if((n = close(srcfd)) < 0)
		fprintf(stderr, "close srcfd error\n");
	return n;
}

// 忽略SIGPIPE信号，防止因为错误的写操作（向读端关闭的socket中写数据）而导致server退出
void handle_for_sigpipe() {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    // signal设置的信号句柄只能起一次作用，信号被捕获一次后，信号句柄就会被还原成默认值了
    // sigaction设置的信号句柄，可以一直有效，值到你再次改变它的设置
    if(sigaction(SIGPIPE, &sa, NULL)) return;
}


// 将打开的fd设置为非阻塞
int setSocketNonBlocking(int fd) {
    // 获取文件的flag，即open函数的第二个参数
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1) return -1;

    //将文件设置为非阻塞
    //nonblock:在读取不到数据或是写入缓冲区已满会马上return，而不会阻塞等待
    flag |= O_NONBLOCK;
    // 设置文件的flags
    if(fcntl(fd, F_SETFL, flag) == -1) return -1;
    return 0;
}


// 设置TCP连接nodelay
void setSocketNoDelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}

// FIXME:“优雅”或“从容关闭”
// 直到所剩数据发送完毕或超时才关闭连接
void setSocketNoLinger(int fd) {
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*)&linger_, 
                sizeof(linger_));
}

//FIXME:优雅关闭
//函数发起FIN报文（shutdown参数须传入SHUT_WR或者SHUT_RDWR才会发送FIN）
void shutDownWR(int fd) {
    shutdown(fd, SHUT_WR);
    // print("shutdown\n");
}

// 使用给定的端口设置套接字，返回监听到的套接字
int socket_bind_listen(int port) {
    // 检查port，取正确区间范围
    if(port < 0 || port > 65535) return -1;

    // 创建socket(IPV4 + TCP)，返回监听描述符
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) return -1;

    // 消除bind时"Address already in use"错误
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, 
                sizeof(optval)) == -1) {
        close(listen_fd);
        return -1;
    }

    // 设置服务器IP和port，和监听描述符绑定
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(listen_fd);
        return -1;
    }

    // 开始监听，最大等待队列长为LISTENQ
    if(listen(listen_fd, 2048) == -1) {
        close(listen_fd);
        return -1;
    }

    // 无效监听描述符
    if (listen_fd == -1) {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}


int Epoll_create(int size) {
	int n;
	if((n = epoll_create(size)) < 0)
		perror("epoll create error");
	return n;
}

int Epoll_create1(int flags) {
	int n;
	if((n = epoll_create1(flags)) < 0)
		perror("epoll create error");
	return n;
}

int Epoll_ctl(int epfd, int op, int fd, epoll_event *event) {
	int n;
	if((n = epoll_ctl(epfd, op, fd, event)) < 0)
		perror("epoll ctl error");
	return n;
}

int Epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout) {
	int n;
	if((n = epoll_wait(epfd, events, maxevents, timeout)) < 0)
		perror("epoll wait error");
	return n;
}

int Eventfd(unsigned int initval, int flags){
	int n;
	if((n = eventfd(initval, flags)) < 0)
		perror("eventfd error");
	return n;
}






