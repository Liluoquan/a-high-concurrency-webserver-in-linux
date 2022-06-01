// @Author: Lawson
// @Date: 2022/03/18

#pragma once
#include <stdint.h>


// CurrentThread不是一个类，而是一个命名空间，目的是提供对于当前线程的管理操作。
namespace CurrentThread {
    // internal
    // __thread修饰变量每一个线程有一个独立实体，各个线程的值互不干扰
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char* t_threadName;

    void cacheTid();

    inline int tid() {
        // 告诉gcc该语句为假的概率很大
        if(__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }

    // for logging
    inline const char* tidString() {
        return t_tidString;
    }

    // for logging
    inline int tidStringLength() {
        return t_tidStringLength;
    }

    inline const char* name() {
        return t_threadName;
    }
}
