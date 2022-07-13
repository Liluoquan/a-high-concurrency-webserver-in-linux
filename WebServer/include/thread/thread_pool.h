#ifndef THREAD_THREAD_POOL_H_
#define THREAD_THREAD_POOL_H_

#include <pthread.h>

#include <functional>
#include <memory>
#include <vector>

#include "utility/noncopyable.h"

namespace thread {
//线程池状态
enum ThreadPoolState {
    THREAD_POOL_INVALID,
    THREAD_POOL_LOCK_FAILURE,
    THREAD_POOL_QUEUE_FULL,
    THREAD_POOL_SHUTDOWN,
    THREAD_POOL_THREAD_FAILURE,
    THREAD_POOL_GRACEFUL
};

typedef enum { 
    immediate_shutdown, 
    graceful_shutdown 
} ShutDownOption;

const int kMaxThreadNum = 1024;
const int kMaxQueueSize = 65535;

struct Task {
    std::function<void(std::shared_ptr<void>)> callback;
    std::shared_ptr<void> args;
};

//线程池(用于计算线程)
class ThreadPool : utility::NonCopyAble {
 public:
    ThreadPool(int thread_num, int queue_size);
    ~ThreadPool();

    int Start();
    int Add(std::shared_ptr<void> args, 
            std::function<void(std::shared_ptr<void>)> task);
    int Destroy(ShutDownOption shutdown_option = graceful_shutdown);
    int Free();

 private:
    static void* Worker(void* args);
    void Run();

 private:
    int thread_num_;
    int queue_size_;
    int head_;
    int tail_;
    int count_;
    int started_thread_count_;
    int shutdown_;

    std::vector<pthread_t> threads_;
    std::vector<Task> task_queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t condition_;
};

}  // namespace thread

#endif  // THREAD_THREAD_POOL_H_