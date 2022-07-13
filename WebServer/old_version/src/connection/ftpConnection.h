// @Author: Lawson
// @CreateDate: 2022/05/14
// @LastModifiedDate: 2022/05/15

#pragma once

#include "../manager/fileOperator.h"
#include "../reactor/Channel.h"
#include "../manager/userManager.h"

#include <vector>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <algorithm> // for transform command
#include <sys/types.h>
#include <sys/socket.h>
#include <functional>

// 命令分隔符
#define SEPARATOR " "

enum ProcessState {
    //LOGIN_ERROR = 0,    // 登录错误（密码或用户名错误）
    QUIT = 0,
    UPLOAD,
    LOGIN,
    PARSE,
};


class ftpConnection {
public:
    using CallBack = std::function<ProcessState()>;
public:
    ftpConnection(sp_Channel ftpChannel, unsigned int connId, std::string defaultDir, unsigned short commandOffset = 1);
    virtual ~ftpConnection();

    std::vector<std::string> extractParameters(std::string command);
    void run();
    int getFD();
    size_t getConnectionId();

private:
    std::shared_ptr<Channel> ftpChannel_;
    ProcessState processState_;
    userManager::userManagerPtr userManager_;
    std::string dir_;
    std::shared_ptr<fileOperator> fo_; // For browsing, writing and reading


    CallBack callBackFunc[4];

    size_t filePart_;
    std::vector<std::string> directories_;
    std::vector<std::string> files_;
    size_t connectionId_;
    std::string parameter_;

    void sendToClient(std::string response);
    bool commandEquals(std::string a, std::string b);
    std::string filterOutBlanks(std::string inString);
    static void getAllParametersAfter(std::vector<std::string> parameterVector, unsigned int currentParameter, std::string& theRest);
    unsigned short commandOffset_;
    std::string inBuffer_;
    std::string outBuffer_;


    ProcessState login();
    ProcessState quit();
    ProcessState commandParser();
    ProcessState upload();
    ProcessState download();

    void handleRead();
    void handleWrite();
};


