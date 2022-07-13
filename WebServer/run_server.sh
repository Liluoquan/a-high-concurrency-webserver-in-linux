#!/bin/bash

#服务器绑定端口号
port=8888               
#日志路径
log_file_name=./server.log
#是否打开日志
open_log=0 
#日志也输出到标准错误流
log_to_stderr=1
#输出日志颜色
color_log_to_stderr=1
#打印的最小日志等级
min_log_level=0
#数据库连接池
connection_num=6
#线程池
thread_num=6

#运行
./bin/web_server -p $port -t $thread_num -f $log_file_name \
                 -o $open_log -s $log_to_stderr \
                 -c $color_log_to_stderr -l $min_log_level
