// @Author: Lawson
// @CreateDate: 2022/05/14
// @LastModifiedDate: 2022/05/15

#include "ftpConnection.h"

#define USERINFOFILENAME "user.txt"
#define BUFFER_SIZE 4096


ftpConnection::ftpConnection(sp_Channel ftpChannel, unsigned int connId, std::string defaultDir,
                             unsigned short commandOffset)
                             : ftpChannel_(ftpChannel), processState_(LOGIN), userManager_(userManager::getUserManger(USERINFOFILENAME)),
                             dir_(defaultDir), fo_(new fileOperator(dir_)), filePart_(0),
                             connectionId_(connId), parameter_(""), commandOffset_(commandOffset)
{
    // std::cout << "Connection to client '" << hostAddress_
    //           << "' established" << std::endl;

    callBackFunc[ProcessState::QUIT] = std::bind(&ftpConnection::quit, this);
    callBackFunc[ProcessState::UPLOAD] = std::bind(&ftpConnection::upload, this);
    callBackFunc[ProcessState::LOGIN] = std::bind(&ftpConnection::login, this);
    callBackFunc[ProcessState::PARSE] = std::bind(&ftpConnection::commandParser, this);

    ftpChannel_->setReadHandler(std::bind(&ftpConnection::handleRead, this));
    ftpChannel_->setEvents(EPOLLIN | EPOLLOUT);
}

ftpConnection::~ftpConnection() {
    // std::cout << "Connection terminated to client (connection id "
    //           << connectionId_ << ")" << std::endl;
    directories_.clear();
    files_.clear();
}

// 比较两个cmd是否相同（不区分大小写）
bool ftpConnection::commandEquals(std::string a, std::string b) {
    std::transform(a.begin(), a.end(), a.begin(), tolower);
    size_t pos = a.find(b);
    return pos != std::string::npos;
}

// 从客户端请求中提取命令和参数
std::vector<std::string> ftpConnection::extractParameters(std::string command) {
    std::vector<std::string> res;
    size_t prepos = 0, pos = 0;
    // 找分隔符
    if((pos  = command.find(SEPARATOR, prepos)) != std::string::npos) {
        res.emplace_back(command.substr(prepos, pos - prepos));
        // std::cout << "Command: " << res.back();
    }
    if(command.size() > pos + 1) {
        res.emplace_back(command.substr(pos + 1, command.size() - (pos + commandOffset_)));
        // std::cout << " - Parameter: '" << res.back() << "'" << std::endl;
    }

    return res;
}


void ftpConnection::sendToClient(std::string response) {
    size_t bytesSend = 0;
    size_t length = response.size();
    int fd = ftpChannel_->getFd();

    // std::cout << "sendToclient now " << response << std::endl;

    while(bytesSend < length) {
        int ret = send(fd, response.c_str() + bytesSend, length - bytesSend, 0);
        if(ret <= 0) {
            return;
        }
        bytesSend += ret;
    }
}


// 基于状态机的读操作
void ftpConnection::handleRead() {
    // 这大概是我写过最简洁的代码了 (:
    processState_ = callBackFunc[processState_]();
    if(processState_ == ProcessState::QUIT) {
        callBackFunc[processState_]();
    }
}

ProcessState ftpConnection::login(/*std::string userInfo*/) {
    LOG << "read id and key";
    std::string userInfo;
    bool zero = false;
    ssize_t readSum = readn(ftpChannel_->getFd(), userInfo, zero);

    LOG << "userInfo receive from client is: " << userInfo;

    if(readSum < 0 || zero) {
        LOG << "readn error";
        // ftpChannel_->setDeleted(true);
        // ftpChannel_->getLoop().lock()->addTimer(ftpChannel_, 0);
        // return;
        return ProcessState::QUIT;
    }
    userInfo.resize(userInfo.size() - commandOffset_ + 1);
    LOG << "userInfo after offset is: " << userInfo;
    // 密码接收格式：%id%key
    size_t f_pos = userInfo.find("%");
    size_t s_pos = userInfo.find("%", f_pos + 1);

    std::string userName = userInfo.substr(f_pos + 1, s_pos - f_pos - 1);
    std::string userPwd = userInfo.substr(s_pos + 1);

#ifdef DEBUG
    LOG << "user's name:" << userName << "  "
        << "user's pwd:"  << userPwd;
#endif

    std::string answer;
    // FIXME: test now! -> true
    if(userManager_->login(userName, userPwd)) { // 密码正确
        answer = "UserInfo correct and enter your cmd :\n";
        // TODO:根据后续指令进行文件处理
        // sendToClient("correct");
        writen(ftpChannel_->getFd(), answer);
        return ProcessState::PARSE;
    }
    else { // 密码错误
        // sendToClient("incorrect");
        answer = "your userName or key is incorrect\n";
        writen(ftpChannel_->getFd(), answer);
        return ProcessState::QUIT;
        // answer = "incorrect";
        // if(writen(ftpChannel_->getFd(), answer)) {
        //     LOG << "writen error";
        // }
        // ftpChannel_->setDeleted(true);
        // ftpChannel_->getLoop().lock()->addTimer(ftpChannel_, 0);
        // return;
    }
}

ProcessState ftpConnection::quit() {
    LOG << "quit() now.";
    sendToClient("connection closed now\n");
    ftpChannel_->setDeleted(true);
    ftpChannel_->getLoop().lock()->addTimer(ftpChannel_, 0);
    return ProcessState::QUIT;
}

ProcessState ftpConnection::commandParser() {
    string command;
    bool zero = false;
    ssize_t readSum = readn(ftpChannel_->getFd(), command, zero);

    LOG << "command receive from client is: " << command;

    if(readSum < 0 || zero) {
        LOG << "readn error";
        // ftpChannel_->setDeleted(true);
        // ftpChannel_->getLoop().lock()->addTimer(ftpChannel_, 0);
        // return;
        return ProcessState::QUIT;
    }

    std::string res;
    // TODO:获取文件信息
    // struct stat status;
    // 每个cmd可以有0 or 1个参数
    // like "browse" or "browse ./"
    std::vector<std::string> commandAndParameter(extractParameters(command));
    // for (int i = 0; i < commandAndParameter.size(); i++) 
    // std::cout << "P " << i << ":" << commandAndParameter[i] << ":" << std::endl;

    std::cout << "Connection " << connectionId_ << ": ";
    std::string cmd = commandAndParameter[0];
    if(commandAndParameter.size() == 1) {
        // TODO:处理没有参数的命令
        if(commandEquals(cmd, "list") || commandEquals(cmd, "ls")) {  // 列出目录
            std::string curDir = "./";
            std::cout << "Browsing files of the current working dir" << std::endl;
            directories_.clear();
            files_.clear();
            fo_->browse(curDir, directories_, files_);

            // 输出目录
            for(std::string& directory : directories_) {
                res += directory + "/ ";
                // std::cout << directory << "/ ";
            }
            std::cout << std::endl;
            if(!res.empty()) {
                res += "\n";
            }
            // 输出文件
            for(std::string& file : files_) {
                res += file + " ";
                // std::cout << file << " ";
            }
            // std::cout << std::endl;
        }
        else if(commandEquals(cmd, "pwd")) { // 输出当前工作目录
            std::cout << "Working dir requested" << std::endl;
            res = fo_->getCurrentWorkingDir();
        }
        else if(commandEquals(cmd, "getparentdir")) { // 获取父目录
            std::cout << "Parent dir of working dir requested" << std::endl;
            res = fo_->getParentDir();
        }
        else if(commandEquals(cmd, "quit")) { // 退出当前连接
            std::cout << "Shutdown of connection requested" << std::endl;
            return ProcessState::QUIT;
        }
        else { // unknown cmd
            std::cout << "Unknown command encountered!" << std::endl;
            commandAndParameter.clear();
            // sendToClient("Unknown command! \n");
            std::string errorMsg = "Unknown command! \n";
            writen(ftpChannel_->getFd(), errorMsg);
            return ProcessState::PARSE;
        }
    }
    else if(commandAndParameter.size() > 1) {
        // TODO:处理多个参数的命令
        parameter_ = commandAndParameter[1];
        if(commandEquals(cmd, "ls")) {
            std::string curDir = parameter_;
            std::cout << "Browsing files of directory '" << curDir << "'" << std::endl;
            directories_.clear();
            files_.clear();
            fo_->browse(curDir, directories_, files_);

            // 输出目录
            for(std::string& directory : directories_) {
                res += directory + "/ ";
                // std::cout << directory << "/ ";
            }
            std::cout << std::endl;
            if(!res.empty()) {
                res += "\n";
            }
            // 输出文件
            for(std::string& file : files_) {
                res += file + " ";
                // std::cout << file << " ";
            }
            // std::cout << std::endl;
        }
        else if(commandEquals(cmd, "download")) {  // 下载文件
            // downloadCommand_ = true;
            std::cout << "Preparing download of file '" << parameter_ << "'" << std::endl;
            return this->download();
        }
        else if(commandEquals(cmd, "upload")) {  // 上传文件（在hanlderead中处理）
            // 先touch一下文件
            // fo_->createAndCoverFile(parameter_);
            // FIXME:不 touch 了，直接用 trunc 打开一个
            std::string filePath = fo_->getCurrentWorkingDir(false).append(parameter_);
            std::cout << "filePath is : " << filePath << std::endl;
            fo_->currentOpenFile_.open(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
            // fo_->currentOpenFile_.close();
            std::cout << "Preparing download of file '" << parameter_ << "'" << std::endl;
            return ProcessState::UPLOAD;
        }
        else if(commandEquals(cmd, "cd")) { // 切换目录
            std::cout << "Change of working dir to '" << parameter_ << "' requested" << std::endl;
            if (!fo_->changeDir(parameter_)) {
                std::cout << "Directory change to '" << parameter_ << "' successful!" << std::endl;
            }
            res = fo_->getCurrentWorkingDir();
        }
        else if(commandEquals(cmd, "mkdir")) { // 创建目录
            std::cout << "Creating of dir '" << parameter_ << "' requested" << std::endl;
            res = (fo_->createDirectory(parameter_) ? "//" : parameter_);
        }
        else if(commandEquals(cmd, "rmdir")) {
            std::cout << "Deletion of dir '" << parameter_ << "' requested" << std::endl;
            if(fo_->dirIsBelowServerRoot(parameter_)) {
                std::cerr << "Attempt to delete directory beyond server root (prohibited)" << std::endl;
                res = "failure";
            }
            else {
                directories_.clear();
                fo_->clearListOfDeletedDirectories();
                files_.clear();
                fo_->clearListOfDeletedFiles();
                if(fo_->deleteDirectory(parameter_)) {
                    std::cerr << "Error when trying to delete directory '" << parameter_ << "'" << std::endl;
                }
                directories_ = std::move(fo_->getListOfDeletedDirectories());
                files_ = std::move(fo_->getListOfDeletedFiles());
                res += "deleted list : \n";
                for(const std::string& directory : directories_) {
                    res += directory + "/ ";
                }
                res += "\n";
                for(const std::string& file : files_) {
                    res += file + " ";
                }
            }
        }
        else if(commandEquals(cmd, "touch")) { // 创建文件
            std::cout << "Creating of empty file '" << parameter_ << "' requested" << std::endl;
            res = (fo_->createFile(parameter_) ? "//" : parameter_);
        }
        else {  // 不存在的指令
            std::cout << "Unknown command encountered!" << std::endl;
            commandAndParameter.clear();
            // sendToClient("Unknown command! \n");
            std::string errorMsg = "Unknown command! \n";
            writen(ftpChannel_->getFd(), errorMsg);
            return ProcessState::PARSE;
        }
    }
    else if(!commandAndParameter[0].empty()) {
        std::cout << "Unknown command encountered!" << std::endl;
        std::cout << std::endl;
        commandAndParameter.clear();
        // sendToClient("Unknown command! \n");
        std::string errorMsg = "Unknown command! \n";
        writen(ftpChannel_->getFd(), errorMsg);
        return ProcessState::PARSE;
    }
    res += "\n";
    LOG << "res is: " << res;
    // sendToClient(res);
    writen(ftpChannel_->getFd(), res);
    return ProcessState::PARSE;
}

ProcessState ftpConnection::upload() {
    string clientFile;
    bool zero = false;
    ssize_t readSum = readn(ftpChannel_->getFd(), clientFile, zero);

    LOG << "clientFile receive from client is: " << clientFile;

    if(clientFile.compare("\r\n") == 0) {
        fo_->currentOpenFile_.close();
        filePart_ = 0;
        std::cout << "write success" << std::endl;
        return ProcessState::PARSE;
    }

    if(readSum < 0 || zero) {
        LOG << "readn error";
        fo_->currentOpenFile_.close();
        return ProcessState::QUIT;
    }

    if(fo_->currentOpenFile_.is_open()) {
        fo_->currentOpenFile_ << clientFile;
        std::cout << "receive part " << ++filePart_ << " of file ! " << std::endl;
        return ProcessState::UPLOAD;
    }

    std::cout << "write failed" << std::endl;

    return ProcessState::QUIT;
}

ProcessState ftpConnection::download() {
    std::string filePath = fo_->getCurrentWorkingDir(false).append(parameter_);
    std::cout << "filePath in download() is " << filePath << std::endl;

    // FIXME:好像只能传输文本文件
    // FILE* fp = fopen(filePath.c_str(), "r");
    // if(fp == nullptr) {
    //     std::cout << "Can not open the file!" << std::endl;
    //     std::string errorMsg = "Can not open the file! \n";
    //     writen(ftpChannel_->getFd(), errorMsg);
    //     return ProcessState::PARSE;
    // }
    // char buffer[BUFFER_SIZE];
    // bzero(buffer, sizeof(buffer));
    // int readBytes = 0;
    // while((readBytes = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
    //     std::cout << "read " << readBytes << " bytes from file " << std::endl;
    //     std::string fileContent(buffer);
    //     if(writen(ftpChannel_->getFd(), fileContent) != readBytes) {
    //         std::cout << "somthing go wrong while sending file " << std::endl;
    //         return ProcessState::QUIT;
    //     }
    //     bzero(buffer, sizeof(buffer));
    // }

    fo_->currentOpenReadFile_.open(filePath, std::ios::binary | std::ios::in);
    if(!fo_->currentOpenReadFile_.is_open()) {
        fo_->currentOpenReadFile_.close();
        std::cout << "Open file failed " << std::endl;
        sendToClient("Open file failed, ensure you enter a correct fileName \n");
        return ProcessState::PARSE;
    }

    char buffer[BUFFER_SIZE];
    int readBytes = 0;
    while(fo_->currentOpenReadFile_.read(buffer, sizeof(buffer))) {
        readBytes = fo_->currentOpenReadFile_.gcount();
        writen(ftpChannel_->getFd(), buffer, readBytes);
    }

    fo_->currentOpenReadFile_.close();
    std::cout << "download() finished " << std::endl;
    return ProcessState::PARSE;
}



