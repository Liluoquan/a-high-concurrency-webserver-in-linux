// @Author: Lawson
// @CreatedDate: 2022/03/28
// @ModifiedDate: 2022/04/20

#include "httpConnection.h"
#include "../package/Util.h"
#include "../reactor/EventLoop.h"
#include "../reactor/Timer.h"
#include "../reactor/Channel.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h> //内存管理声明
#include <sys/stat.h> //文件状态
#include <iostream>

using namespace std;

// 定义静态成员
pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime_;
// std::unordered_map<int, std::string> MimeType::ErrorMsg_;

const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET /* | EPOLLONESHOT */;
const int DEFAULT_EXPIRED_TIME = 2000;             //ms
const int DEFAULT_KEEP_ALIVE_TIME = 1 * 60 * 1000; //ms(1 min)

void MimeType::init()
{

    mime_[".html"] = "text/html";
    mime_[".avi"] = "video/x-msvideo";
    mime_[".bmp"] = "image/bmp";
    mime_[".c"] = "text/plain";
    mime_[".doc"] = "application/msword";
    mime_[".gif"] = "image/gif";
    mime_[".gz"] = "application/x-gzip";
    mime_[".htm"] = "text/html";
    mime_[".ico"] = "image/x-icon";
    mime_[".jpg"] = "image/jepg";
    mime_[".png"] = "image/png";
    mime_[".txt"] = "text/plain";
    mime_[".mp3"] = "audio/mp3";
    mime_["default"] = "text/html";

    // 错误码初始化
    // ErrorMsg_[404] = "Not Found";
    // ErrorMsg_[400] = "Bad Request";
}

std::string MimeType::getMime(const std::string &suffix)
{
    pthread_once(&once_control, MimeType::init); // 保证MimeType::init()在同一进程仅执行一次
    if (mime_.find(suffix) == mime_.end())
        return mime_["default"];
    else
        return mime_[suffix];
}


httpConnection::httpConnection(sp_Channel channel)
    : channel_(channel),
      nowReadPos_(0),
      parseState_(ParseState::PARSE_REQUSET),
      basePath_("page/"),
      method_(HttpMethod::METHOD_GET),
      HTTPVersion_(HttpVersion::HTTP_11),
      keepAlive_(false)
{
    handleParse_[0] = bind(&httpConnection::parseError, this);
    handleParse_[1] = bind(&httpConnection::parseRequest, this);
    handleParse_[2] = bind(&httpConnection::parseHeader, this);
    handleParse_[3] = bind(&httpConnection::parseSuccess, this);
    channel_->setReadHandler(bind(&httpConnection::handleRead, this));
    channel_->setEvents(EPOLLIN | EPOLLET);
    LOG << "create a httpConnection";
}

void httpConnection::reset()
{
    fileName_.clear();
    //path_.clear();
    nowReadPos_ = 0;
    // parseState_ = PARSE_REQUSET;
    headers_.clear();
}

// 从inBuffer_中找到从nowReadPos_开始的第一个str
bool httpConnection::findStrFromBuffer(string &msg, string str)
{
    size_t pos = inBuffer_.find(str, nowReadPos_);
    if (pos == string::npos)
        return false;
    msg = inBuffer_.substr(nowReadPos_, pos - nowReadPos_);
    nowReadPos_ = pos + str.size();

    return true;
}

// 每次读取到信息就进行基于状态机的解析
void httpConnection::handleRead()
{
    LOG << "read data";
    bool zero = false;
    ssize_t readSum = readn(channel_->getFd(), inBuffer_, zero);

    if (readSum < 0 || zero)
    {
        LOG << "readn error";
        // FIXME:此处包含了读到RST和FIN的情况，暂时按照断开连接处理
        this->reset();
        channel_->setDeleted(true);
        channel_->getLoop().lock()->addTimer(channel_, 0);
        return;
    }

    LOG << "inBuffer_: " << inBuffer_;

    while (inBuffer_.size() > 0 &&
           inBuffer_.find("\r\n", nowReadPos_) != string::npos)
    {
        parseState_ = handleParse_[parseState_]();
    }
}

ParseState httpConnection::parseRequest()
{
    string msg;
    if (!findStrFromBuffer(msg, " /")) {
        errorState_ = ErrorState::BAD_REQUEST;
        return ParseState::PARSE_ERROR;
    }
    else if (msg == "GET")
        this->method_ = HttpMethod::METHOD_GET;
    else if (msg == "POST")
        this->method_ = HttpMethod::METHOD_POST;
    else {
        errorState_ = ErrorState::BAD_REQUEST;
        return ParseState::PARSE_ERROR;
    }

    if (!findStrFromBuffer(fileName_, " ")) {
        errorState_ = ErrorState::BAD_REQUEST;
        return ParseState::PARSE_ERROR;
    }

    struct stat sbuf;
    if (fileName_.empty())
        fileName_ = std::move("index.html");

    if (stat((basePath_ + fileName_).c_str(), &sbuf) < 0)
    {
        LOG << "no such file: " << (basePath_ + fileName_);
        errorState_ = ErrorState::NOT_FOUND;
        // parseState_ = PARSE_ERROR;
        return ParseState::PARSE_ERROR;
    }

    fileSize_ = sbuf.st_size;
    if (!findStrFromBuffer(msg, "\r\n")) {
        errorState_ = ErrorState::BAD_REQUEST;
        return ParseState::PARSE_ERROR;
    }
    else if (msg == "HTTP/1.0")
        this->HTTPVersion_ = HttpVersion::HTTP_10;
    else if (msg == "HTTP/1.1")
        this->HTTPVersion_ = HttpVersion::HTTP_11;
    else {
        errorState_ = ErrorState::BAD_REQUEST;
        return ParseState::PARSE_ERROR;
    }

    return ParseState::PARSE_HEADER;
}

ParseState httpConnection::parseHeader()
{
    if (inBuffer_[nowReadPos_] == '\r' && inBuffer_[nowReadPos_ + 1] == '\n')
        return ParseState::PARSE_SUCCESS;
    string key, value;
    if (!findStrFromBuffer(key, ": ") || !findStrFromBuffer(value, "\r\n")) {
        // ErrorNo_ = 400;
        errorState_ = ErrorState::BAD_REQUEST;
        return ParseState::PARSE_ERROR;
    }

    headers_[key] = value;
    return ParseState::PARSE_HEADER;
}

ParseState httpConnection::parseError()
{
    LOG << "parse error";
    inBuffer_.clear();
    nowReadPos_ = 0;
    this->reset();
    headers_.clear();

    // 调用epoll_ctl将fd重新注册到epoll事件池，用以触发EPOLLOUT事件
    // string ErrorMsg = MimeType::getErrorMsg(ErrorNo_);

    switch(errorState_) {
        case NOT_FOUND : {
            // TODO:直接重定向到另一个html页面
            channel_->setWriteHandler(bind(&httpConnection::handleError, this, NOT_FOUND, "Not Found"));
            break;
        }
        case BAD_REQUEST : {
            channel_->setWriteHandler(bind(&httpConnection::handleError, this, BAD_REQUEST, "Bad Request"));
            break;
        }
        default : {
            channel_->setWriteHandler(bind(&httpConnection::handleError, this, BAD_REQUEST, "Bad Request"));
        }
    }

    channel_->setEvents(EPOLLOUT | EPOLLET);
    channel_->getLoop().lock()->updatePoller(channel_);

    return ParseState::PARSE_REQUSET;
}

ParseState httpConnection::parseSuccess()
{  
    inBuffer_ = inBuffer_.substr(nowReadPos_ + 2);
    nowReadPos_ = 0;

    if (HTTPVersion_ == HTTP_11 && headers_.count("Connection") != 0 && (headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "Keep-alive"))
    {
        keepAlive_ = true;
    }

    size_t dot_pos = fileName_.find('.');
    if (dot_pos == string::npos)
        this->fileType_ = MimeType::getMime("default");
    else
        this->fileType_ = MimeType::getMime(fileName_.substr(dot_pos));

    channel_->setWriteHandler(bind(&httpConnection::handleSuccess, this));
    // 调用epoll_ctl将fd重新注册到epoll事件池，用以触发EPOLLOUT事件
    channel_->setEvents(EPOLLOUT | EPOLLET);
    channel_->getLoop().lock()->updatePoller(channel_);

    headers_.clear();
    return ParseState::PARSE_REQUSET;
}

void httpConnection::handleSuccess()
{
    outBuffer_.clear();
    if (method_ == METHOD_POST)
    {
        //TODO:处理POST请求
    }
    else if (method_ == METHOD_GET)
    {
        outBuffer_ += "HTTP/1.1 200 OK\r\n";
        if (keepAlive_ == true)
        {
            outBuffer_ += "Connection: Keep-Alive\r\n";
            outBuffer_ += "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
            // 长连接的处理
            channel_->getLoop().lock()->addTimer(channel_, DEFAULT_KEEP_ALIVE_TIME);
        }
        else
        {
            // 短连接的处理
            channel_->getLoop().lock()->addTimer(channel_, 0);
        }
        outBuffer_ += "Content-Type: " + fileType_ + "\r\n";
        outBuffer_ += "Content-Length: " + to_string(fileSize_) + "\r\n";
        outBuffer_ += "Server: Lawson's HttpServer\r\n";
        outBuffer_ += "\r\n";

        // 先在缓存中找
        if(!(getCache().get(fileName_, outBuffer_))) {

            LOG << "Cache miss!";
            int src_fd = Open((basePath_ + fileName_).c_str(), O_RDONLY, 0);
            char *src_addr = (char *)mmap(NULL, fileSize_, PROT_READ, MAP_PRIVATE, src_fd, 0);
            Close(src_fd);
            string context(src_addr, fileSize_);
            outBuffer_ += context;

            // 将文件加入缓存
            getCache().set(fileName_, context);
            munmap(src_addr, fileSize_);
        }
        else {
            LOG << "Cache hit!";
        }
    }
    
    // LOG << "outBuffer_:" << outBuffer_;

    if(writen(channel_->getFd(), outBuffer_) <= 0) {
        LOG << "writen error";
    }

    // 在长连接中复用HttpData
    this->reset();

    // 重新注册读事件
    channel_->setEvents(EPOLLIN | EPOLLET);
    channel_->getLoop().lock()->updatePoller(channel_);

}

void httpConnection::handleError(int err_num, string short_msg)
{
    //short_msg = " " + short_msg;
    //char send_buff[4096];
    string body_buff;
    body_buff += "<html><title>sorry, it's an error!</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + " " + short_msg;
    body_buff += "<hr><em> Lawson's WebServer</em>\n</body></html>";

    outBuffer_ += "HTTP/1.1 " + to_string(err_num) + " " + short_msg + "\r\n";
    outBuffer_ += "Content-Type: text/html\r\n";
    outBuffer_ += "Connection: Close\r\n";
    outBuffer_ += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
    outBuffer_ += "Server: Lawson's HttpServer\r\n";
    outBuffer_ += "\r\n";

    outBuffer_ += body_buff;

    if(writen(channel_->getFd(), outBuffer_) <= 0) {
        LOG << "writen error";
    }

    outBuffer_.clear();

    // 重新注册读事件
    channel_->setEvents(EPOLLIN | EPOLLET);
    channel_->getLoop().lock()->updatePoller(channel_);
}
