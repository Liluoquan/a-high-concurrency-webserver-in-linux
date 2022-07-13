#ifndef UTILITY_SOCKET_UTILS_H_
#define UTILITY_SOCKET_UTILS_H_

#include <cstdlib>
#include <string>

namespace utility {

//设置NIO非阻塞套接字 read时不管是否读到数据 都直接返回，如果是阻塞式，就会等待直到读完数据或超时返回
//所以非阻塞需要一直去看现在是否有数据 有就读出来 没有就立即返回
int SetSocketNonBlocking(int fd);
//禁用TCP Nagle算法 这样就不会存到写缓存积累一定量再发送 而是直接发送 
void SetSocketNoDelay(int fd);
//优雅关闭套接字 也就是套接字在close的时候是否等待缓冲区发送完成(内核默认close是直接返回的)
void SetSocketNoLinger(int fd);

//shutdown只关闭了连接 close则关闭了套接字
//shutdown会等输出缓冲区中的数据传输完毕再发送FIN报文段 而close则直接关闭 将会丢失输出缓冲区的内容
void ShutDownWR(int fd);
//注册处理管道信号的回调 对其信号屏蔽
void HandlePipeSignal();

//服务器绑定地址并监听端口
int SocketListen(int port);

//从fd中读n个字节到buffer
int Read(int fd, void* read_buffer, int size);
int Read(int fd, std::string& read_buffer, bool& is_read_zero_bytes);
int Read(int fd, std::string& read_buffer);

//从buffer中写n个字节到fd
int Write(int fd, void* write_buffer, int size);
int Write(int fd, std::string& write_buffer);

}  // namespace utility

#endif  // UTILITY_SOCKET_UTILS_H_
