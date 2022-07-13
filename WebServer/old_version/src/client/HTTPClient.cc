// @Author: Lawson
// @CreateDate: 2022/04/10
// @LastModifiedDate: 2022/04/10

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

using namespace std;

#define MAXSIZE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 8888
#define FDSIZE 1024
#define EPOLLEVENTS 20

int setSocketNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL);
    if(flag == -1) {
        return -1;
    }

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1) {
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    // cout << "creating socket..." << endl;
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1) {
        perror("create socket error!\n");
        return 0;
    }
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, IPADDRESS, &servaddr.sin_addr);
    char buff[4096];
    buff[0] = '\0';
    cout << "-------------------" << endl;
    const char* p;
    ssize_t n;
    // 发送空串
    p = "  \r\n";
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("connect to server failed!");
        return 0;
    }
    setSocketNonBlocking(sockfd);
    cout << "test1: " << endl;
    n = write(sockfd, p, strlen(p));
    cout << "strlen(p) = " << strlen(p) << endl;
    sleep(1);
    n = read(sockfd, buff, sizeof(buff));
    cout << "n = " << n << endl;
    cout << buff << endl;
    close(sockfd);
    sleep(1);

    // // 发"GET /page.jpg HTTP/1.1"
    // p = "GET /page.jpg HTTP/1.1\r\nHost: 192.168.52.135\r\n\r\n";
    // sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // if(sockfd == -1) {
    //     perror("create socket error");
    //     return 0;
    // }
    // if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
    //     perror("connect to server failed");
    //     return 0;
    // }
    // setSocketNonBlocking(sockfd);
    // cout << "test2: " << endl;
    // n = write(sockfd, p, strlen(p));
    // cout << "strlen(p) = " << strlen(p) << endl;
    // sleep(1);
    // n = read(sockfd, buff, sizeof(buff));
    // cout << "n = " << n << endl;
    // cout << buff << endl;
    // close(sockfd);
    // sleep(1);

    // 发
    // GET /index.html HTTP/1.1
    // Host: 192.168.52.135:8888
    // Content-Type: application/x-www-form-urlencoded
    // Connection: Keep-Alive
    for(int i = 0; i < 3; ++i) {
        p = "GET /index.html HTTP/1.1\r\nHost: 192.168.52.135\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: Keep-Alive\r\n\r\n";
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(sockfd == -1) {
            perror("create socket error");
            return 0;
        }
        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
            perror("connect to server failed");
            return 0;
        }
        setSocketNonBlocking(sockfd);
        cout << "test3: " << endl;
        n = write(sockfd, p, strlen(p));
        cout << "strlen(p) = " << strlen(p) << endl;
        sleep(1);
        n = read(sockfd, buff, sizeof(buff));
        cout << "n = " << n << endl;
        cout << buff << endl;
        close(sockfd);
        sleep(1);
    }


    return 0;
}
