// @Author: Lawson
// @CreateDate: 2022/05/06
// @LastModifiedDate: 2022/05/06

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
#include "../package/Util.h"

using namespace std;

#define MAXSIZE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 4242
#define FDSIZE 1024
#define EPOLLEVENTS 20

#define DEBUG_FTPCLIENT


int main(int argc, char* argv[]) {
    std::cout << "it's FTPClient!" << std::endl;
    char buff[4096];
    buff[0] = '\0';

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1) {
        cout << "create socket error!\n";
        return 0;
    }

    // setSocketNonBlocking(sockfd);
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, IPADDRESS, &serverAddr.sin_addr);

    std::string user_name, user_password;
    std::string msg;

#ifndef DEBUG_FTPCLIENT
    std::cout<<"please input your user name and password seperated by space.\n";
    std::cin >> user_name >> user_password;
#else
    user_name = "admin";
    user_password = "123456";
#endif

    msg = "%" + user_name + "%" + user_password;

    if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cout << "connect to server failed!" << std::endl;
        return 0;
    }
    setSocketNonBlocking(sockfd);

#ifdef DEBUG_FTPCLIENT
    cout << "send userInfo now" << std::endl;
#endif
    sleep(1);
    size_t n = write(sockfd, msg.c_str(), msg.size());

#ifdef DEBUG_FTPCLIENT
    cout << "the msg send to server is: " << msg << std::endl;
    // cout << "the msg.c_str() send to server is: " << msg.c_str() << std::endl;
#endif
    sleep(1);
    n = read(sockfd, buff, sizeof(buff));
    msg = buff;

#ifdef DEBUG_FTPCLIENT
    // cout << "the msg receive from server is: " << msg << std::endl;
#endif

    if(msg.find("correct") == std::string::npos) {
        std::cout << "your user name or password is incorrect." << std::endl;
        return 0;
    }

#ifdef DEBUG_FTPCLIENT
    
    std::string cmd;
    bool zero = false;
    readn(sockfd, msg, zero);
    std::cout << "msg1 is: " << msg << std::endl;
    while(true) {
        std::cout << "enter your cmd: " << std::endl;
        getline(std::cin, cmd);
        writen(sockfd, cmd);
        msg.clear();
        ssize_t readSum = readn(sockfd, msg, zero);
        if(readSum < 0 || zero) {
            break;
        }
        std::cout << "msg2 is: " << msg << std::endl;
        
    }

    cout << "FTPClient close" << endl;
#endif
}