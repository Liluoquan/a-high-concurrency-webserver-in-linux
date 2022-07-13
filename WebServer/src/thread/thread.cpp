#include "thread/thread.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <memory>

namespace current_thread {
// TLS
__thread int tls_thread_id = 0;                     //线程id
__thread const char* tls_thread_name = "default";   //线程名字
}  // namespace current_thread

namespace thread {
// Thread类
Thread::Thread(const Worker& worker, const std::string& thread_name)
    : worker_(worker),
      pthread_id_(0),
      thread_id_(0),
      thread_name_(thread_name), 
      is_started_(false),
      is_joined_(false) {
    if (thread_name_.empty()) {
        thread_name_ = "Thread";
    }
}

Thread::~Thread() {
    //如果线程已经开始 但是没有join 就detach线程
    if (is_started_ && !is_joined_) {
        pthread_detach(pthread_id_);
    }
}

//开始线程  调用Run Run会调用ThreadData的Run函数 执行worker线程函数
void Thread::Start() {
    assert(!is_started_);
    is_started_ = true;

    //创建线程并执行线程函数
    if (pthread_create(&pthread_id_, NULL, &Run, this) != 0) {
        is_started_ = false;
    } 
}

//join线程
int Thread::Join() {
    assert(is_started_);
    assert(!is_joined_);
    is_joined_ = true;

    return pthread_join(pthread_id_, NULL);
}

// 调用ThreadData的Run函数 执行worker线程函数
void* Thread::Run(void* arg) {
    Thread* thread = static_cast<Thread*>(arg);
    thread->Run();

    return NULL;
}

void Thread::Run() { 
    //赋值线程id
    thread_id_ = current_thread::thread_id();
    //设置线程名
    current_thread::tls_thread_name = thread_name_.empty() ? "Thread" : thread_name_.c_str();
    prctl(PR_SET_NAME, current_thread::tls_thread_name);

    //执行线程函数
    worker_();
    current_thread::tls_thread_name = "finished";
}

}  // namespace thread
