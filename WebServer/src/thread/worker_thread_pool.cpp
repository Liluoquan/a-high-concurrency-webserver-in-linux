#include "thread/worker_thread_pool.h"
#include "thread/block_queue.hpp"

namespace thread {
WorkerThreadPool::WorkerThreadPool(int thread_num)
    : pthreads_id_(NULL),
      thread_num_(thread_num) {
    if (thread_num <= 0) {
	    exit(1);
    }
    // 线程id初始化
    pthreads_id_ = new pthread_t[thread_num_];
    if (!pthreads_id_) {
	    exit(1);
    }

    for (int i = 0; i < thread_num; ++i) {
        // 创建线程 tid,线程属性attr,线程函数入口func,线程函数参数arg 
        if (pthread_create(pthreads_id_ + i, NULL, Worker, this) != 0) {
            delete[] pthreads_id_;
			exit(1);
        }
        // 分离线程 后面不用再对线程进行回收
        if (pthread_detach(pthreads_id_[i])) {
            delete[] pthreads_id_;
			exit(1);
        }
    }
}

// 释放线程池
WorkerThreadPool::~WorkerThreadPool() {
    delete[] pthreads_id_;
}

// 线程处理函数
void* WorkerThreadPool::Worker(void* arg) {
    WorkerThreadPool* thread_pool = (WorkerThreadPool*)arg;
    thread_pool->Run();
    
    return NULL;
}

// 执行任务 
void WorkerThreadPool::Run() {
    while (true) {
        auto channels = SingletonBlockQueue::GetInstance()->Pop(); 
        for (auto channel : channels) {  
            //处理每个就绪事件(不同channel绑定了不同的callback)
            if (channel) {
                channel->HandleEvents();
            }
        }
    }
}

}  // namespace thread
