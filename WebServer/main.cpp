#include <getopt.h>

#include "server/web_server.h"
#include "event/event_loop.h"
#include "log/logging.h"

namespace configure {
//默认值
static int port = 8888;
static int thread_num = 8;
static std::string log_file_name = "./web_server.log";
static bool open_log = true;
static bool log_to_stderr = false;
static bool color_log_to_stderr = false;
static int min_log_level = INFO;

static void ParseArg(int argc, char* argv[]) {
    int opt;
    const char* str = "p:t:f:o:s:c:l:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p': {
                port = atoi(optarg);
                break;
            }
            case 't': {
                thread_num = atoi(optarg);
                break;
            }
            case 'f': {
                log_file_name = optarg;
                break;
            }
            case 'o': {
                open_log = atoi(optarg);
                break;
            }
            case 's': {
                log_to_stderr = atoi(optarg);
                break;
            }
            case 'c': {
                color_log_to_stderr = atoi(optarg);
                break;
            }
            case 'l': {
                min_log_level = atoi(optarg);
                break;
            }
            default: {
                break;
            }
        }
    }
}

}  // namespace configure

int main(int argc, char* argv[]) {
    //解析参数
    configure::ParseArg(argc, argv);
    
    // 设置日志文件
    log::Logging::set_log_file_name(configure::log_file_name);
    // 开启日志
    log::Logging::set_open_log(configure::open_log);
    // 设置日志输出标准错误流
    log::Logging::set_log_to_stderr(configure::log_to_stderr);
    // 设置日志输出颜色
    log::Logging::set_color_log_to_stderr(configure::color_log_to_stderr);
    // 设置最小日志等级
    log::Logging::set_min_log_level(configure::min_log_level);

    // 主loop  初始化poller, 把eventfd注册到epoll中并注册其事件处理回调
    event::EventLoop main_loop;

    // 创建监听套接字绑定服务器，监听端口，设置监听套接字为NIO，屏蔽管道信号
    // server::WebServer::GetInstance()->Initialize(&main_loop, configure::thread_num, configure::port);
    server::WebServer myHttpServer(&main_loop, configure::thread_num, configure::port);
    
    // 主loop创建事件循环线程池(子loop),每个线程都run起来（调用SubLoop::Loop）
    // 给监听套接字设置监听事件，绑定事件处理回调，注册到主loop的epoll内核事件表中
    // server::WebServer::GetInstance()->Start();
    myHttpServer.Start();

    // 主loop开始事件循环  epoll_wait阻塞 等待就绪事件(主loop只注册了监听套接字的fd，所以只会处理新连接事件)
    std::cout << "================================================Start Web Server================================================" << std::endl;
    main_loop.Loop();
    std::cout << "================================================Stop Web Server=================================================" << std::endl;

    return 0;
}
