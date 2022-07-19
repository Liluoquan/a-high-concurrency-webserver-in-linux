#include "connection/http_connection.h"

#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

#include "event/channel.h"
#include "event/event_loop.h"
#include "utility/socket_utils.h"
#include "log/logging.h"

namespace http {

// HttpConnection类 给连接套接字绑定事件处理回调
HttpConnection::HttpConnection(event::EventLoop* event_loop, int connect_fd)
    : event_loop_(event_loop),
      connect_fd_(connect_fd),
      connect_channel_(new event::Channel(connect_fd)),
      connection_state_(CONNECTED),
      process_state_(STATE_PARSE_LINE),
      parse_state_(START),
      method_(METHOD_GET),
      version_(HTTP_1_1),
      is_error_(false),
      is_keep_alive_(false) {
    // 给连接套接字绑定读、写、连接的回调函数
    connect_channel_->set_read_handler(std::bind(&HttpConnection::HandleRead, this));
    connect_channel_->set_write_handler(std::bind(&HttpConnection::HandleWrite, this));
    connect_channel_->set_update_handler(std::bind(&HttpConnection::HandleUpdate, this));
    
    // 得到服务器资源目录
    char cwd[100];
    char* cur_dir = getcwd(cwd, 100);
    resource_dir_ = std::string(cur_dir) + "/pages/";
}

// http连接析构时 关闭连接套接字
HttpConnection::~HttpConnection() {
    close(connect_fd_);
}

// 给fd注册默认事件, 这里给了超时时间，所以会绑定定时器和http对象
void HttpConnection::Register() {
    connect_channel_->set_events(kDefaultEvent);
    event_loop_->PollerAdd(connect_channel_, kDefaultTimeOut);
}

// 从epoll事件表中删除fd(绑定的定时器被删除时 会调用此函数),然后http连接释放，会close掉fd
// 注意此时需要延长 HttpConnection 的寿命，避免 HttpConnection 在函数执行过程中主动关闭
// 导致 coredump
void HttpConnection::Delete() {
    // 连接状态变为已关闭
    std::shared_ptr<HttpConnection> guard(shared_from_this());
    connection_state_ = DISCONNECTED;
    event_loop_->PollerDel(connect_channel_);
}

// 处理读   读请求报文数据到 read_buffer 解析请求报文 构建响应报文并写入write_buffer
void HttpConnection::HandleRead() {
    int& events = connect_channel_->events();
    do {
        bool is_read_zero_bytes = false;
        // 读客户端发来的请求数据 存入read_buffer
        int read_bytes = utility::Read(connect_fd_, read_buffer_, is_read_zero_bytes);
        // LOG(DEBUG) << "Request " << read_buffer_;
        if (connection_state_ == DISCONNECTING) {
            read_buffer_.clear();
            break;
        }
        if (read_bytes < 0) {
            LOG(WARNING) << "Read bytes < 0, " << strerror(errno);
            // 标记为error的都是直接返回错误页面
            is_error_ = true;
            ReturnErrorMessage(connect_fd_, 400, "Bad Request");
            break;
        } else if (is_read_zero_bytes) {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            connection_state_ = DISCONNECTING;
            if (read_bytes == 0) {
                break;
            }
        }

        // 解析请求行
        if (process_state_ == STATE_PARSE_LINE) {
            LineState line_state = ParseRequestLine();
            if (line_state == PARSE_LINE_AGAIN) {
                break;
            } else if (line_state == PARSE_LINE_ERROR) {
                LOG(WARNING) << "Parse request line error, sockfd: " 
                             << connect_fd_ << ", " << read_buffer_ << "******";
                read_buffer_.clear();
                // 标记为error的都是直接返回错误页面
                is_error_ = true;
                ReturnErrorMessage(connect_fd_, 400, "Bad Request");
                break;
            } else {
                process_state_ = STATE_PARSE_HEADERS;
            }
        }

        //解析请求头
        if (process_state_ == STATE_PARSE_HEADERS) {
            HeaderState header_state = ParseRequestHeader();
            if (header_state == PARSE_HEADER_AGAIN) {
                break;
            } else if (header_state == PARSE_HEADER_ERROR) {
                LOG(WARNING) << "Parse request header error, sockfd: " 
                             << connect_fd_ << ", " << read_buffer_ << "******";
                //标记为error的都是直接返回错误页面
                is_error_ = true;
                ReturnErrorMessage(connect_fd_, 400, "Bad Request");
                break;
            }
            // 如果是GET方法此时已解析完成 如果是POST方法 继续解析请求体
            if (method_ == METHOD_POST) {
                // POST方法
                process_state_ = STATE_RECV_BODY;
            } else {
                process_state_ = STATE_RESPONSE;
            }
        }

        //post方法
        if (process_state_ == STATE_RECV_BODY) {
            //读content_length字段 看请求体有多少字节的数据
            int content_length = -1;
            if (request_headers_.find("Content-length") != request_headers_.end()) {
                content_length = stoi(request_headers_["Content-length"]);
            } else {
                // 标记为error的都是直接返回错误页面
                is_error_ = true;
                ReturnErrorMessage(connect_fd_, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (read_buffer_.size() < content_length) {
                break;
            }
            process_state_ = STATE_RESPONSE;
        }
        
        //构建响应报文并写入write_buffer
        if (process_state_ == STATE_RESPONSE) {
            ResponseState response_state = BuildResponse();
            if (response_state == RESPONSE_SUCCESS) {
                process_state_ = STATE_FINISH;
                break;
            } else {
                is_error_ = true;
                break;
            }
        }
    } while (false);

    if (!is_error_) {
        //如果成功解析 将响应报文数据发送给客户端
        if (write_buffer_.size() > 0) {
            HandleWrite();
        }
        if (!is_error_ && process_state_ == STATE_FINISH) {
            //完成后 reset状态置为初始化值
            Reset();
            //如果read_buffer还有数据 就调用自己继续读
            if (read_buffer_.size() > 0) {
                if (connection_state_ != DISCONNECTING) {
                    HandleRead();
                }
            }
        } else if (!is_error_ && connection_state_ != DISCONNECTED) {
            //如果没发生错误 对端也没关闭 此次没处理完 就下次再处理
            events |= EPOLLIN;
        }
    }
}

// 处理写  向客户端发送write_buffer中的响应报文数据
void HttpConnection::HandleWrite() {
    //如果没有发生错误 并且连接没断开 就把写缓冲区的数据 发送给客户端
    if (!is_error_ && connection_state_ != DISCONNECTED) {
        int& events = connect_channel_->events();
        // LOG(DEBUG) << "Response " << write_buffer_;
        if (utility::Write(connect_fd_, write_buffer_) < 0) {
            LOG(WARNING) << "Send response to client error, " << strerror(errno);
            events = 0;
            is_error_ = true;
        }
        //如果还没有写完 就等待下次写事件就绪接着写
        if (write_buffer_.size() > 0) {
            events |= EPOLLOUT;
        }
    }
}

// 处理更新事件回调 
void HttpConnection::HandleUpdate() {
    // 删除定时器（后面会重新绑定新定时器，相当于更新到期时间)
    ResetTimer();
    int& events = connect_channel_->events();

    if (!is_error_ && connection_state_ == CONNECTED) {
        // 还处在建立连接状态 如果事件不为0，说明在处理时添加了(EPOLLIN或者EPOLLOUT)
        if (events != 0) {
            // 如果keep-alive 则超时时间就设为5分钟，否则就是5秒
            int timeout = kDefaultTimeOut;
            if (is_keep_alive_) {
                timeout = kDefaultKeepAliveTime;
            }
            // 如果监听事件是读加写，就变为写, 最后用ET边缘触发模式
            if ((events & EPOLLIN) && (events & EPOLLOUT)) {
                events = 0;
                events |= EPOLLOUT;
            }
            events |= EPOLLET;
            // 更新监听事件， 以及重新绑定新定时器, Loop最后调HandleExpire会删掉旧的定时器
            event_loop_->PollerMod(connect_channel_, timeout);
        } else if (is_keep_alive_) {
            // 当前没有事件 并且keep-alive 监听可读事件
            events |= (EPOLLIN | EPOLLET);
            int timeout = kDefaultKeepAliveTime;
            event_loop_->PollerMod(connect_channel_, timeout);
        } else {
            //当前没有事件 并且not keep-alive 监听可读事件
            events |= (EPOLLIN | EPOLLET);
            int timeout = (kDefaultKeepAliveTime >> 1);
            event_loop_->PollerMod(connect_channel_, timeout);
        }

    } else if (!is_error_ && connection_state_ == DISCONNECTING 
                    && (events & EPOLLOUT)) {
        // 对端连接已关闭
        events = (EPOLLOUT | EPOLLET);
    } else {
        // 连接已经关闭, EpollDel，close(fd)
        event_loop_->RunInLoop(std::bind(&HttpConnection::Delete, shared_from_this()));
    }
}

// 处理错误（返回错误信息）
void HttpConnection::ReturnErrorMessage(int fd, int error_code, std::string error_message) {
    error_message = " " + error_message;

    //响应体
    std::string response_body;
    response_body += "<html><head><meta charset=\"utf-8\">";
    response_body += "<title>请求出错了</title></head>";
    response_body += "<body bgcolor=\"ffffff\">";
    response_body += "<div align=\"center\">" + std::to_string(error_code) + error_message;
    response_body += "<hr>lawson's web server\n</div></body></html>";

    //响应头
    std::string response_header;
    response_header += "HTTP/1.1 " + std::to_string(error_code) + error_message + "\r\n";
    response_header += "Content-Type: text/html\r\n";
    response_header += "Connection: Close\r\n";
    response_header += "Content-Length: " + std::to_string(response_body.size()) + "\r\n";
    response_header += "Server: lawson's tiny webserver\r\n";
    response_header += "\r\n";
    
    // 错误处理不考虑write不完的情况
    write_buffer_ = response_header + response_body;
    utility::Write(fd, write_buffer_);
    write_buffer_.clear();
}

// 解析请求行
LineState HttpConnection::ParseRequestLine() {
    std::string& request_data = read_buffer_;
    // 读到完整的请求行再开始解析请求
    size_t pos = request_data.find('\r');
    if (pos < 0) {
        return PARSE_LINE_AGAIN;
    }
    std::string request_line = request_data.substr(0, pos);
    if (request_data.size() > pos + 1) {
        // read_buffer去掉请求行
        request_data = request_data.substr(pos + 1);
    } else {
        request_data.clear();
    }
    // GET /filename HTTP1.1 请求方法
    int get_pos = request_line.find("GET");
    int post_pos = request_line.find("POST");
    int head_pos = request_line.find("HEAD");

    if (get_pos >= 0) {
        pos = get_pos;
        method_ = METHOD_GET;
    } else if (post_pos >= 0) {
        pos = post_pos;
        method_ = METHOD_POST;
    } else if (head_pos >= 0) {
        pos = head_pos;
        method_ = METHOD_HEAD;
    } else {
        return PARSE_LINE_ERROR;
    }

    //GET /filename HTTP1.1 请求文件名
    pos = request_line.find("/", pos);
    if (pos < 0) {
        //没找到 就默认访问主页，http版本默认1.1
        file_name_ = "index.html";
        version_ = HTTP_1_1;
        return PARSE_LINE_SUCCESS;
    } else {
        //找到了文件名 再找空格
        size_t pos2 = request_line.find(' ', pos);
        if (pos2 < 0) {
            return PARSE_LINE_ERROR;
        } else {
            //找到了空格 
            if (pos2 - pos > 1) {
                // /后一个字符到空格前一个字符就是文件名
                file_name_ = request_line.substr(pos + 1, pos2 - pos - 1);
                //?表示如果有参数 只取参数前的请求文件名
                size_t pos3 = file_name_.find('?');
                if (pos3 >= 0) {
                    file_name_ = file_name_.substr(0, pos3);
                }
            } else {
                //找到/并且只有一个字符 那就是/ 默认进入index.html
                file_name_ = "index.html";
            }
        }
        pos = pos2;
    }
    
    //GET /filename HTTP/1.1 版本号
    pos = request_line.find("/", pos);
    if (pos < 0) {
        return PARSE_LINE_ERROR;
    } else {
        if (request_line.size() - pos <= 3) {
            return PARSE_LINE_ERROR;
        } else {
            std::string version = request_line.substr(pos + 1, 3);
            if (version == "1.0") {
                version_ = HTTP_1_0;
            } else if (version == "1.1") {
                version_ = HTTP_1_1;
            } else {
                return PARSE_LINE_ERROR;
            }
        }
    }

    return PARSE_LINE_SUCCESS;
}

//解析请求头
HeaderState HttpConnection::ParseRequestHeader() {
    std::string& request_data = read_buffer_;
    int key_start = -1;
    int key_end = -1;
    int value_start = -1;
    int value_end = -1;
    int now_read_line_begin = 0;
    bool is_finish = false;
    int i = 0;

    //逐字符
    for (; i < request_data.size() && !is_finish; ++i) {
        switch (parse_state_) {
            case START: {
                if (request_data[i] == '\n' || request_data[i] == '\r') {
                    break;
                }
                parse_state_ = KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case KEY: {
                if (request_data[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                    parse_state_ = COLON;
                } else if (request_data[i] == '\n' || request_data[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case COLON: {
                if (request_data[i] == ' ') {
                    parse_state_ = SPACES_AFTER_COLON;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case SPACES_AFTER_COLON: {
                parse_state_ = VALUE;
                value_start = i;
                break;
            }
            case VALUE: {
                if (request_data[i] == '\r') {
                    parse_state_ = CR;
                    value_end = i;
                    if (value_end - value_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                } else if (i - value_start > 255) {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case CR: {
                if (request_data[i] == '\n') {
                    parse_state_ = LF;
                    std::string key(request_data.begin() + key_start, request_data.begin() + key_end);
                    std::string value(request_data.begin() + value_start, request_data.begin() + value_end);
                    request_headers_[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case LF: {
                if (request_data[i] == '\r') {
                    parse_state_ = END_CR;
                } else {
                    key_start = i;
                    parse_state_ = KEY;
                }
                break;
            }
            case END_CR: {
                if (request_data[i] == '\n') {
                    parse_state_ = END_LF;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case END_LF: {
                is_finish = true;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }

    if (parse_state_ == END_LF) {
        request_data = request_data.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    request_data = request_data.substr(now_read_line_begin);
    
    return PARSE_HEADER_AGAIN;
}

// 构建响应报文并写入write_buffer
ResponseState HttpConnection::BuildResponse() {
    if (method_ == METHOD_POST) {
        // POST方法 暂时返回500
        write_buffer_.clear();
        ReturnErrorMessage(connect_fd_, 500, "Server Busy!");
        return RESPONSE_ERROR;
    } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
        // GET | HEAD方法
        std::string response_header;
        response_header += "HTTP/1.1 200 OK\r\n";
        //如果在请求头中找到Connection字段 看是不是keep-alive
        if (request_headers_.find("Connection") != request_headers_.end() && 
                (request_headers_["Connection"] == "Keep-Alive" ||
                 request_headers_["Connection"] == "keep-alive")) {
            is_keep_alive_ = true;
            response_header += std::string("Connection: Keep-Alive\r\n") +
                                           "Keep-Alive: timeout=" + 
                                            std::to_string(kDefaultKeepAliveTime) +
                                            "\r\n";
        }

        // echo test
        if (file_name_ == "echo") {
            //响应体
            write_buffer_ = "HTTP/1.1 200 OK\r\n";
            write_buffer_ += "Content-Type: text/html\r\n\r\nHello World";
            return RESPONSE_SUCCESS;
        }

        // 如果是请求图标 把请求头+图标数据(请求体)写入write_buffer
        // TODO: 加入 favicon.ico 文件
        // if (file_name_ == "favicon.ico") {
        //     response_header += "Content-Type: image/png\r\n";
        //     response_header += "Content-Length: " + std::to_string(sizeof(web_server_favicon)) + "\r\n";
        //     response_header += "Server: lawson's webserver\r\n";
        //     response_header += "\r\n";
        //     write_buffer_ += response_header;
        //     write_buffer_ += std::string(web_server_favicon, web_server_favicon + sizeof(web_server_favicon));
        //     return RESPONSE_SUCCESS;
        // }

        //根据文件类型 来设置mime类型
        int pos = file_name_.find('.');
        std::string file_type;
        if (pos < 0) {
            file_type = MimeType::get_mime("default");
        } else {
            file_type = MimeType::get_mime(file_name_.substr(pos));
        }

        //查看请求的文件权限
        struct stat file_stat;
        //请求文件名 = 资源目录 + 文件名
        file_name_ = resource_dir_ + file_name_;
        //请求的文件没有权限返回403
        if (stat(file_name_.c_str(), &file_stat) < 0) {
            write_buffer_.clear();
            ReturnErrorMessage(connect_fd_, 403, "Forbidden!");
            return RESPONSE_ERROR;
        }
        response_header += "Content-Type: " + file_type + "\r\n";
        response_header += "Content-Length: " + std::to_string(file_stat.st_size) + "\r\n";
        response_header += "Server: lawson's web server\r\n";
        // 请求头后的空行
        response_header += "\r\n";
        write_buffer_ += response_header;

        // 请求HEAD方法 不必返回响应体数据 此时就已经完成 
        if (method_ == METHOD_HEAD) {
            return RESPONSE_SUCCESS;
        }

        // 先在缓存中找
        if (!cache::LFUCache::GetInstance().Get(file_name_, write_buffer_)) {
            // cache miss.
            // 请求的文件不存在返回404
            int file_fd = open(file_name_.c_str(), O_RDONLY, 0);
            if (file_fd < 0) {
                write_buffer_.clear();
                ReturnErrorMessage(connect_fd_, 404, "Not Found!");
                return RESPONSE_ERROR;
            }
            // 将文件内容通过mmap映射到一块共享内存中
            void* mmap_address = mmap(NULL, file_stat.st_size, PROT_READ, 
                                    MAP_PRIVATE, file_fd, 0);
            close(file_fd);
            // 映射共享内存失败 也返回404
            if (mmap_address == (void*)-1) {
                munmap(mmap_address, file_stat.st_size);
                write_buffer_.clear();
                ReturnErrorMessage(connect_fd_, 404, "Not Found!");
                return RESPONSE_ERROR;
            }
            
            // 将共享内存里的内容 写入write_buffer
            char* file_address = static_cast<char*>(mmap_address);
            write_buffer_ += std::string(file_address, file_address + file_stat.st_size);

            // 加入缓存
            std::string context(file_address, file_stat.st_size);
            cache::LFUCache::GetInstance().Set(file_name_, context);

            // 关闭映射
            munmap(mmap_address, file_stat.st_size);
        }


        
        return RESPONSE_SUCCESS;
    }

    return RESPONSE_ERROR;
}

// 重置HTTP状态 
void HttpConnection::Reset() {
    process_state_ = STATE_PARSE_LINE;
    parse_state_ = START;
    file_name_.clear();
    request_headers_.clear();
    
    // 删除定时器 为了重新加入新定时器到堆中
    if (timer_.lock()) {
        auto timer = timer_.lock();
        timer->Release();
        timer_.reset();
    }
}

//删除定时器,为了重新加入新定时器到堆中
void HttpConnection::ResetTimer() {
    if (timer_.lock()) {
        auto timer = timer_.lock();
        timer->Release();
        timer_.reset();
    }
}

}  // namespace http
