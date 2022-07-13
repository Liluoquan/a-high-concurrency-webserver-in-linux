#ifndef THREAD_WORKER_THREAD_POOL_H_
#define THREAD_WORKER_THREAD_POOL_H_

#include <pthread.h>

#include <event/poller.h>

namespace thread {
class WorkerThreadPool {
public:
    /*thread_num是线程池中线程的数量，max_request是请求队列中最多允许的、等待处理的请求的数量*/
    WorkerThreadPool(int thread_num);
    ~WorkerThreadPool();
    
    void set_poller(event::Poller* poller) {
        poller_ = poller;
    }

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void* Worker(void* arg);
    void Run();

private:
    int thread_num_;           //线程池中的线程数
    pthread_t* pthreads_id_;   //工作线程池，其大小为thread_num
    event::Poller* poller_;
};

}  // namespace thread

#endif  // THREAD_WORKER_THREAD_POOL_H_
