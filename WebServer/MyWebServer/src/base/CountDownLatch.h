// @Author: Lawson
// @Date: 2022/03/17

#pragma once
#include "Condition.h"
#include "MutexLock.h"
#include "noncopyable.h"

// CountDownLatch是一个同步辅助类，一个线程阻塞等待另一个线程完成之后再继续进行(可以理解为等待起跑的指令)
// 此处仿照muduo::CountDownLatch而写


class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();
    int getCount() const;
    
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};
