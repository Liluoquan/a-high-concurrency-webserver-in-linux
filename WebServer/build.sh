#!/bin/bash

#用makefile还是cmake
use_make=0
build_dir=./build

if [ $use_make -eq 1 ]
then
    #Makefile
    make clean && make -j8 && make install
else
    #cmake 判断文件夹是否存在
    if [ ! -d $build_dir ]
    then
        mkdir $build_dir 
    else
        rm -r $build_dir
        mkdir $build_dir 
    fi

    cd $build_dir
    cmake .. && make -j8 && make install 
    cd ..
fi

flush_core
rm -f $log_file_name
