#!/bin/bash

webbench=./bin/web_bench
#服务器地址
server_ip=127.0.0.1
server_port=8888
url=http://${server_ip}:${server_port}/echo

#子进程数量
process_num=1000
#请求时间(单位s)
request_time=60
#keep-alive
is_keep_alive=1
#force
is_force=0

#命令行选项
options="-c $process_num -t $request_time $url"

if [ $is_force -eq 1 ]
then
    options="-f $options" 
fi 

if [ $is_keep_alive -eq 1 ]
then
    options="-k $options" 
fi 

#删除之前开的进程
#kill -9 `ps -ef | grep web_bench | awk '{print $2}'`

#运行
$webbench $options


