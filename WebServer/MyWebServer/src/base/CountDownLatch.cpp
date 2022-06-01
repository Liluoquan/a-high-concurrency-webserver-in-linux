// @Author: Lawson
// @Date: 2022/03/17

#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) { }

void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);
    // Condition_在wait()的时候会将锁释放
    while(count_ > 0) condition_.wait();
}


void CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_);
    --count_;
    if(count_ == 0) condition_.notifyAll();
}

int CountDownLatch::getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
}
