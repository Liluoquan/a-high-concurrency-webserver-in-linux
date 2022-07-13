#ifndef HTTP_HTTP_TYPE_H_
#define HTTP_HTTP_TYPE_H_

#include <sys/epoll.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <unordered_map>

#include "timer/timer.h"

namespace http {
//图标
extern char web_server_favicon[555];

//http mime文件类型
class MimeType {
 public:
    static std::string get_mime(const std::string& type);

 private:
    MimeType();
    MimeType(const MimeType& mime_type);

    static void OnceInit();

 private:
    static std::unordered_map<std::string, std::string> mime_map;
    static pthread_once_t once_control;
};

//处理状态
enum ProcessState {
    STATE_PARSE_LINE,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_RESPONSE,
    STATE_FINISH
};

//解析uri（请求行）的状态
enum LineState {
    PARSE_LINE_SUCCESS,
    PARSE_LINE_AGAIN,
    PARSE_LINE_ERROR,
};

//解析请求头的状态
enum HeaderState {
    PARSE_HEADER_SUCCESS,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

//构建响应报文时的状态
enum ResponseState { 
    RESPONSE_SUCCESS,
    RESPONSE_ERROR 
};

//解析状态
enum ParseState {
    START,
    KEY,
    COLON,
    SPACES_AFTER_COLON,
    VALUE,
    CR,
    LF,
    END_CR,
    END_LF
};

//服务器和客户端的连接状态
enum ConnectionState { 
    CONNECTED, 
    DISCONNECTING, 
    DISCONNECTED 
};

//Http请求方法
enum RequestMethod { 
    METHOD_GET, 
    METHOD_POST, 
    METHOD_HEAD 
};

//Http版本
enum HttpVersion { 
    HTTP_1_0, 
    HTTP_1_1 
};

}  // namespace http

#endif  // HTTP_HTTP_TYPE_H_