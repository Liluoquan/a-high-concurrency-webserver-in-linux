// 实现对条件变量pthread_cond_t的封装，但是time暂时不知道作用是什么
// @Author: Lawson
// @Date: 2022/03/17

#pragma once
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <cstdint>
#include "MutexLock.h"
#include "noncopyable.h"

class Condition : noncopyable {
public:
    explicit Condition(MutexLock &_mutex) : mutex(_mutex) {
        pthread_cond_init(&cond, NULL);
    }
    ~Condition() { pthread_cond_destroy(&cond); }

    // 使当前线程阻塞在cond上，pthread_cond_wait()分为以下三部
    // 1、线程放在等待队列上，解锁
    // 2、等待 pthread_cond_signal或者pthread_cond_broadcast信号之后去竞争锁
    // 3、若竞争到互斥锁则加锁
    void wait() { pthread_cond_wait(&cond, mutex.get()); }
    // 解除线程在条件变量上的阻塞
    void notify() { pthread_cond_signal(&cond); }
    // 解除所有线程在条件变量上的阻塞
    void notifyAll() { pthread_cond_broadcast(&cond); }
    // 阻塞直到指定时间
    bool waitForSeconds(int seconds) {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        // 超时返回的错误码是ETIMEDOUT
        return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
    }

private:
    MutexLock &mutex;
    pthread_cond_t cond;
};
