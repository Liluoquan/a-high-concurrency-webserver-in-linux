#ifndef THREAD_BLOCK_QUEUE_H_
#define THREAD_BLOCK_QUEUE_H_

#include <queue>
#include <vector>

#include "locker/mutex_lock.h"
#include "event/channel.h"

namespace thread {
// 阻塞队列模板类
template <typename T>
class BlockQueue {
 public:
    BlockQueue();
    ~BlockQueue();

    void Push(T* task);
    void Push(const std::vector<T*>& tasks);
    void Push(std::vector<T*>&& tasks);
    std::vector<T*> Pop();
    bool Empty();
    int Size();
    // T* Front();

    //静态成员函数 线程安全的懒汉式单例模式
    static BlockQueue<T>* GetInstance() {
        static BlockQueue<T> block_queue;
        return &block_queue;
    }

 private:
    std::queue<std::vector<T*>> task_queue_;
    mutable locker::MutexLock mutex_;   //请求队列的互斥锁
    locker::ConditionVariable condition_; //请求队列的条件变量
};

template <typename T>
BlockQueue<T>::BlockQueue()
    : mutex_(),
      condition_(mutex_) {
}

template <typename T>
BlockQueue<T>::~BlockQueue() {
}

template <typename T>
void BlockQueue<T>::Push(T* task) {
    locker::LockGuard lock(mutex_);
    // 添加任务
    if (task) {
        task_queue_.push(task);
        // 唤醒
        condition_.notify();
    }
}

template <typename T>
void BlockQueue<T>::Push(const std::vector<T*>& tasks) {
    locker::LockGuard lock(mutex_);
    // for (auto task : tasks) {
    //     // 添加任务
    //     task_queue_.push(task);
    // }
    task_queue_.push(tasks);
    // 唤醒
    condition_.notify();
}

template <typename T>
void BlockQueue<T>::Push(std::vector<T*>&& tasks) {
    locker::LockGuard lock(mutex_);
    // for (auto task : tasks) {
    //     // 添加任务
    //     task_queue_.push(task);
    // }
    task_queue_.push(tasks);
    // 唤醒
    condition_.notify();
}

template <typename T>
std::vector<T*> BlockQueue<T>::Pop() {
    locker::LockGuard lock(mutex_);
    //如果队列为空，条件变量等待
    while (task_queue_.empty()) {
        condition_.wait();
    } 
    // 从请求队列中取出一个任务
    auto task = task_queue_.front();
    task_queue_.pop();
    return task;
}

template <typename T>
bool BlockQueue<T>::Empty() {
    locker::LockGuard lock(mutex_);
    return task_queue_.empty();
}

template <typename T>
int BlockQueue<T>::Size() {
    locker::LockGuard lock(mutex_);
    return task_queue_.size();
}

// template <typename T>
// T* BlockQueue<T>::Front() {
//     locker::LockGuard lock(mutex_);
//     if (task_queue_.empty()) {
//         return NULL;
//     } else {
//         return task_queue_.front();
//     }
// }

}  // namespace thread

typedef thread::BlockQueue<event::Channel> SingletonBlockQueue;

#endif  // BLOCK_QUEUE_H_