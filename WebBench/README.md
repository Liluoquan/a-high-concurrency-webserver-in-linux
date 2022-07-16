# 开源压力测试工具WebBench

## 介绍
* WebBench是Web性能压力测试工具。使用fork()系统调用创建多个子进程，模拟客户端去访问指定服务器。
* 支持HTTP/0.9, HTTP/1.0, HTTP1.1请求，最终输出: 请求/秒，字节/秒。
* 本项目使用C++11实现，核心实现在web_bench.cpp，可以生成静态库和动态库提供api调用，详细的中文注释。
* 修复了WebBench connect()失败时sockfd泄漏的bug，以及读取响应报文时读完了依然read导致阻塞的bug(因为是BIO，读完了再读就会阻塞了)。
* 支持HTTP1.1 Connection: keep-alive。

## 环境
* OS: Ubuntu 18.04
* Compiler: g++ 7.5.0
* Makefile: 4.1 

## 构建
* 使用Makefile来build
    make -j8 && make install

* 直接运行脚本
    ./build.sh

## 运行
    ./web_bench [-c process_num] [t request_time] url 

## 还有其他命令行选项：
* -k:           keep-alive
* -f:           不等待服务器响应
* -f:           不等待服务器响应
* -r:           重新请求加载(无缓存)
* -9:           使用http0.9协议来构造请求 
* -1:           使用http1.0协议来构造请求 
* -2:           使用http1.1协议来构造请求 
* -k:           客户端是否支持keep alive
* -t:           运行时间，单位：秒，默认为30秒
* -c:           创建多少个客户端，默认为1个 
* -p:           使用代理服务器发送请求 
* --get:        使用GET请求方法 
* --head        使用HEAD请求方法 
* --options     使用OPTIONS请求方法 
* --trace       使用TRACE请求方法 
* -?|-h|--help  显示帮助信息 
* -V|--version  显示版本信息



