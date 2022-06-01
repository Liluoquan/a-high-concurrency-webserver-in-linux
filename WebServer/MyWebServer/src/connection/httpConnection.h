// @Author: Lawson
// @Date: 2022/03/28

#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "../reactor/Timer.h"
#include "../base/noncopyable.h"
#include "../LFUCache/LFUCache.h"

class EventLoop;
class TimerNode;
class Channel;

typedef std::shared_ptr<Channel> sp_Channel;

// 请求行中的方法
enum HttpMethod {
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};

enum HttpVersion {
    HTTP_10 = 1,
    HTTP_11
};

// HTTP请求解析状态
/*
enum ParseState {
    STATE_PARSE_URI = 1, // 处于解析URI的状态
    STATE_PARSE_HEADERS, // 处于解析请求头部的状态
    STATE_RECV_BODY,     // 处于解析请求实体的状态
    STATE_ANALYSIS,      // 处于处理请求的状态
    STATE_FINISH         // 解析完毕
};
*/
enum ParseState {
    PARSE_ERROR = 0,    // 解析错误
    PARSE_REQUSET,      // 解析请求行
    PARSE_HEADER,       // 解析头部
    PARSE_SUCCESS       // 解析成功
};

// 错误状态
enum ErrorState{
    NOT_FOUND = 404,
    BAD_REQUEST = 400
};

// 连接状态
enum ConnectionState {
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED
};

// 建立后缀名与MIME的映射
class MimeType : noncopyable {
private:
    static void init();
    static std::unordered_map<std::string, std::string> mime_;
    // static std::unordered_map<int, std::string> ErrorMsg_;
    static pthread_once_t once_control;
    // MimeType();
    // MimeType(const MimeType& m);
public:
    static std::string getMime(const std::string& suffix);
    // static std::string getErrorMsg(const int& ErrorNo);
};



class httpConnection {
    typedef std::function<ParseState()> CallBack;
public:
    httpConnection(sp_Channel channel);
    ~httpConnection() {}
    void reset();
    // void seperateTimer();
    std::shared_ptr<Channel> getChannel() { return channel_; }
    // EventLoop* getLoop() { return loop_; }
    // void handleClose();
    // void newEvent();
    
private:
    std::shared_ptr<Channel> channel_;
    int nowReadPos_;
    ParseState parseState_;
    std::string inBuffer_;
    std::string outBuffer_;
    std::string basePath_;  // 基本路径

    // 存放解析请求的结果
    HttpMethod method_;
    HttpVersion HTTPVersion_;
    bool keepAlive_;
    std::string fileName_;
    std::string fileType_;
    int fileSize_;
    ErrorState errorState_;
    std::map<std::string, std::string> headers_; // 建立头部字段的映射

    CallBack handleParse_[4];

    void handleRead();
    void handleError(int err_num, std::string short_msg);
    void handleSuccess();

    ParseState parseRequest();
    ParseState parseHeader();
    ParseState parseError();
    ParseState parseSuccess();

    bool findStrFromBuffer(std::string& msg, std::string str);
};

typedef std::shared_ptr<httpConnection> sp_HttpConnection;









