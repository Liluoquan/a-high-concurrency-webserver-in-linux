#include "thread/thread_pool.h"

namespace thread {

ThreadPool::ThreadPool(int thread_num, int queue_size)
    : thread_num_(thread_num),
      queue_size_(queue_size),
      head_(0),
      tail_(0),
      count_(0),
      shutdown_(0),
      started_thread_count_(0),
      mutex_(PTHREAD_MUTEX_INITIALIZER),
      condition_(PTHREAD_COND_INITIALIZER) {
    if (thread_num_ <= 0 
            || thread_num_ > kMaxThreadNum) {
        thread_num_ = 6;
    } 
    if (queue_size_ <= 0 
            || queue_size_ > kMaxQueueSize) {
        queue_size_ = 1024;
    }
    
    threads_.resize(thread_num_);
    task_queue_.resize(queue_size_);
}

int ThreadPool::Start() {
    for (int i = 0; i < thread_num_; ++i) {
        if (pthread_create(&threads_[i], NULL, Worker, this) != 0) {
            return -1;
        }
        ++started_thread_count_;
    }

    return 0;
}

int ThreadPool::Add(std::shared_ptr<void> args,
                    std::function<void(std::shared_ptr<void>)> callback) {
    int next, is_error = 0;
    if (pthread_mutex_lock(&mutex_) != 0) {
        return THREAD_POOL_LOCK_FAILURE;
    }

    do {
        next = (tail_ + 1) % queue_size_;
        // 队列满
        if (count_ == queue_size_) {
            is_error = THREAD_POOL_QUEUE_FULL;
            break;
        }
        // 已关闭
        if (shutdown_) {
            is_error = THREAD_POOL_SHUTDOWN;
            break;
        }
        task_queue_[tail_].callback = callback;
        task_queue_[tail_].args = args;
        tail_ = next;
        ++count_;

        /* pthread_cond_broadcast */
        if (pthread_cond_signal(&condition_) != 0) {
            is_error = THREAD_POOL_LOCK_FAILURE;
            break;
        }
    } while (false);

    if (pthread_mutex_unlock(&mutex_) != 0) {
        is_error = THREAD_POOL_LOCK_FAILURE;
    }
    return is_error;
}

int ThreadPool::Destroy(ShutDownOption shutdown_option) {
    printf("Thread pool destroy !\n");
    int i, is_error = 0;

    if (pthread_mutex_lock(&mutex_) != 0) {
        return THREAD_POOL_LOCK_FAILURE;
    }
    do {
        if (shutdown_) {
            is_error = THREAD_POOL_SHUTDOWN;
            break;
        }
        shutdown_ = shutdown_option;

        if ((pthread_cond_broadcast(&condition_) != 0) ||
            (pthread_mutex_unlock(&mutex_) != 0)) {
            is_error = THREAD_POOL_LOCK_FAILURE;
            break;
        }

        for (i = 0; i < thread_num_; ++i) {
            if (pthread_join(threads_[i], NULL) != 0) {
                is_error = THREAD_POOL_THREAD_FAILURE;
            }
        }
    } while (false);

    if (!is_error) {
        Free();
    }
    
    return is_error;
}

int ThreadPool::Free() {
    if (started_thread_count_ > 0) {
        return -1;
    }
    pthread_mutex_lock(&mutex_);
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&condition_);
    
    return 0;
}

void* ThreadPool::Worker(void* args) {
    auto thread_pool = (ThreadPool*)args;
    thread_pool->Run();

    return NULL;
}

void ThreadPool::Run() {
    while (true) {
        Task task;
        pthread_mutex_lock(&mutex_);
        while ((count_ == 0) && (!shutdown_)) {
            pthread_cond_wait(&condition_, &mutex_);
        }
        if ((shutdown_ == immediate_shutdown) ||
            ((shutdown_ == graceful_shutdown) && (count_ == 0))) {
            break;
        }
        task.callback = task_queue_[head_].callback;
        task.args = task_queue_[head_].args;
        task_queue_[head_].callback = NULL;
        task_queue_[head_].args.reset();
        head_ = (head_ + 1) % queue_size_;
        --count_;
        pthread_mutex_unlock(&mutex_);

        (task.callback)(task.args);
    }

    --started_thread_count_;
    pthread_mutex_unlock(&mutex_);
    printf("This threadpool thread finishs!\n");
    pthread_exit(NULL);
}

}  // namespace thread
