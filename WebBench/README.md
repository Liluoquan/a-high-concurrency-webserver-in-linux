# 开源压力测试工具WebBench

## 介绍
* `WebBench` 是开源的 `Web` 性能压力测试工具。使用 `fork()` 系统调用创建多个子进程，模拟客户端去访问指定服务器。
* 支持 `HTTP/0.9`, `HTTP/1.0`, `HTTP1.1` 请求，最终输出: 请求/秒，字节/秒。
* 本项目使用 `C++11` 实现，核心实现在 `web_bench.cpp`，可以生成静态库和动态库提供 `api` 调用，详细的中文注释。
* 修复了 `WebBench` `connect()` 失败时 `sockfd` 泄漏的bug，以及读取响应报文时读完了依然read导致阻塞的bug(因为是BIO，读完了再读就会阻塞了)。
* 支持 `HTTP1.1 Connection: keep-alive`。

## 环境
- 硬件平台：阿里云 轻量应用服务器 （双核4G，6M 带宽）
- OS version：Ubuntu 18.04
- Complier version: g++ 7.4.0 (Ubuntu 7.4.0-1ubuntu1~18.04)
- cmake version: 3.10.2
- Debugger version: gdb (Ubuntu 8.1.1-0ubuntu1) 8.1.1

## 构建
```shell
# 使用Makefile来build
make -j8 && make install
# 使用脚本构建
./build.sh
```

## 运行
```shell
# 直接运行
./web_bench [-c process_num] [t request_time] url

# 使用脚本运行
sh ./run_bench.sh
```

## 还有其他命令行选项：
* `-k`:            keep-alive
* `-f`:            不等待服务器响应
* `-r`:            重新请求加载(无缓存)
* `-9`:            使用http0.9协议来构造请求 
* `-1`:            使用http1.0协议来构造请求 
* `-2`:            使用http1.1协议来构造请求 
* `-k`:            客户端是否支持keep alive
* `-t`:            运行时间，单位：秒，默认为30秒
* `-c`:            创建多少个客户端，默认为1个 
* `-p`:            使用代理服务器发送请求 
* `--get`:         使用GET请求方法 
* `--head`:        使用HEAD请求方法 
* `--options`:     使用OPTIONS请求方法 
* `--trace`:       使用TRACE请求方法 
* `-?|-h|--help`:  显示帮助信息 
* `-V|--version`:  显示版本信息



