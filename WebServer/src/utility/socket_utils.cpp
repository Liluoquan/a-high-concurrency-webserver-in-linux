#include "utility/socket_utils.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log/logging.h"

namespace utility {
//最大buffer size
const int MAX_BUFFER_SIZE = 4096;

//设置NIO非阻塞套接字 read时不管是否读到数据 都直接返回，如果是阻塞式，就会等待直到读完数据或超时返回
int SetSocketNonBlocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL, 0);
    if (old_flag == -1) {
        return -1;
    }

    int new_flag = old_flag | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, new_flag) == -1) {
        return -1;
    }

    return 0;
}

//Nagle算法通过减少需要传输的数据包来优化网络（也就是尽可能发送一次发送大块数据），在内核中 数据包的发送接收都会先缓存
//启动TCP_NODELAY就禁用了Nagle算法，应用于延时敏感型任务（要求实时性，传输数据量小）
//TCP默认应用Nagle算法,数据只有在写缓存累计到一定量时(也就是多次写)，才会被发送出去 明星提高了网络利用率，但是增加了延时
void SetSocketNoDelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

//优雅关闭套接字 也就是套接字在close的时候是否等待缓冲区发送完成
void SetSocketNoLinger(int fd) {
    struct linger linger_struct;
    linger_struct.l_onoff = 1;
    linger_struct.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*)&linger_struct, sizeof(linger_struct));
}

//shutdown只关闭了连接 close则关闭了套接字
//shutdown会等输出缓冲区中的数据传输完毕再发送FIN报文段 而close则直接关闭 将会丢失输出缓冲区的内容
void ShutDownWR(int fd) {
    //SHUT_WR断开输出流 
    shutdown(fd, SHUT_WR);
}

//注册处理管道信号的回调 对其信号屏蔽
void HandlePipeSignal() {
    struct sigaction signal_action;
    memset(&signal_action, '\0', sizeof(signal_action));
    signal_action.sa_handler = SIG_IGN;
    signal_action.sa_flags = 0;
    if (sigaction(SIGPIPE, &signal_action, NULL)) {
        return;
    }
}

//服务器绑定地址并监听端口
int SocketListen(int port) {
    // 检查port值，取正确区间范围
    if (port < 0 || port > 65535) {
        LOG(FATAL) << "Port error";
    }

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        LOG(FATAL) << "Create listen socket error, " << strerror(errno);
    }

    // 消除bind时"Address already in use"错误
    int flag = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
        LOG(FATAL) << "Set socket option error, " << strerror(errno);
        close(listen_fd);
    }

    // 绑定服务器的ip和端口
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    //IPv4协议
    server_addr.sin_family = AF_INET;                   
    //0.0.0.0(也就是本机所有ip地址) 如果一个主机有多个网卡 每个网卡都连接一个网络 就有多个ip了 这里可以都绑定
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    //主机字节序转网络字节序  
    server_addr.sin_port = htons((unsigned short)port);
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        LOG(FATAL) << "Bind address error, " << strerror(errno);
        close(listen_fd);
    }

    // 开始监听端口，最大等待队列长为LISTENQ
    if (listen(listen_fd, 2048) == -1) {
        LOG(FATAL) << "Listen port error, " << strerror(errno);
        close(listen_fd);
    }

    // 无效监听描述符
    if (listen_fd == -1) {
        LOG(FATAL) << "Invalid listen socket, " << strerror(errno);
        close(listen_fd);
    }

    return listen_fd;
}

//从fd中读n个字节到buffer
int Read(int fd, void* read_buffer, int size) {
    int read_bytes = 0;
    int read_sum_bytes = 0;
    char* buffer = (char*)read_buffer;

    //epoll ET模式 直到读到EAGAIN为止
    while (size > 0) {
        if ((read_bytes = read(fd, buffer, size)) < 0) {
            //EINTR是中断引起的 所以重新读就行
            if (errno == EINTR) {
                read_bytes = 0;
            } else if (errno == EAGAIN) {
                //EAGAIN表明数据读完了 
                return read_sum_bytes;
            } else {
                return -1;
            }
        } else if (read_bytes == 0) {
            break;
        }

        read_sum_bytes += read_bytes;
        size -= read_bytes;
        buffer += read_bytes;
    }

    return read_sum_bytes;
}

//从fd中读n个字节到buffer
int Read(int fd, std::string& read_buffer, bool& is_read_zero_bytes) {
    int read_bytes = 0;
    int read_sum_bytes = 0;

    //epoll ET模式 直到读到EAGAIN为止
    while (true) {
        //每次都读到这个buffer里去
        char buffer[MAX_BUFFER_SIZE];
        if ((read_bytes = read(fd, buffer, MAX_BUFFER_SIZE)) < 0) {
            //EINTR是中断引起的 所以重新读就行
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                //EAGAIN表明数据读完了 
                return read_sum_bytes;
            } else {
                LOG(ERROR) << "Read from sockfd error, " << strerror(errno);
                return -1;
            }
        } else if (read_bytes == 0) {
            is_read_zero_bytes = true;
            break;
        }
        
        read_sum_bytes += read_bytes;
        read_buffer += std::string(buffer, buffer + read_bytes);
    }

    return read_sum_bytes;
}

//从fd中读n个字节到buffer
int Read(int fd, std::string& read_buffer) {
    int read_bytes = 0;
    int read_sum_bytes = 0;

    //epoll ET模式 直到读到EAGAIN为止
    while (true) {
        char buffer[MAX_BUFFER_SIZE];
        if ((read_bytes = read(fd, buffer, MAX_BUFFER_SIZE)) < 0) {
            //EINTR是中断引起的 所以重新读就行
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                //EAGAIN表明数据读完了 
                return read_sum_bytes;
            } else {
                LOG(ERROR) << "Read from sockfd error, " << strerror(errno);
                return -1;
            }
        } else if (read_bytes == 0) {
            break;
        }

        read_sum_bytes += read_bytes;
        read_buffer += std::string(buffer, buffer + read_bytes);
    }

    return read_sum_bytes;
}

//从buffer中写n个字节到fd
int Write(int fd, void* write_buffer, int size) {
    int write_bytes = 0;
    int write_sum_bytes = 0;
    char* buffer = (char*)write_buffer;

    //epoll ET模式 直到写到EAGAIN为止
    while (size > 0) {
        if ((write_bytes = write(fd, buffer, size)) <= 0) {
            if (write_bytes < 0) {
               //EINTR是中断引起的 所以重新写就行
                if (errno == EINTR) {
                    write_bytes = 0;
                    continue;
                } else if (errno == EAGAIN) {
                    //EAGAIN表明缓冲区数据写完了 
                    return write_sum_bytes;
                } else {
                    return -1;
                }
            }
        }

        write_sum_bytes += write_bytes;
        size -= write_bytes;
        buffer += write_bytes;
    }

    return write_sum_bytes;
}

//从buffer中写n个字节到fd
int Write(int fd, std::string& write_buffer) {
    int size = write_buffer.size();
    int write_bytes = 0;
    int write_sum_bytes = 0;
    const char* buffer = write_buffer.c_str();

    //epoll ET模式 直到写到EAGAIN为止
    while (size > 0) {
        if ((write_bytes = write(fd, buffer, size)) <= 0) {
            //EINTR是中断引起的 所以重新写就行
            if (write_bytes < 0) {
                if (errno == EINTR) {
                    write_bytes = 0;
                    continue;
                } else if (errno == EAGAIN) {
                    //EAGAIN表明缓冲区数据写完了 
                    break;
                } else {
                    return -1;
                }
            }
        }

        write_sum_bytes += write_bytes;
        size -= write_bytes;
        buffer += write_bytes;
    }

    if (write_sum_bytes == write_buffer.size()) {
        write_buffer.clear();
    } else {
        write_buffer = write_buffer.substr(write_sum_bytes);
    }

    return write_sum_bytes;
}

}  // namespace utility
