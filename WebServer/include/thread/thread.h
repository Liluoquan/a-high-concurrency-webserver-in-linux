#ifndef THREAD_THREAD_H_
#define THREAD_THREAD_H_

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include <string>
#include <memory>
#include <functional>

#include "locker/mutex_lock.h"
#include "utility/count_down_latch.h"
#include "utility/noncopyable.h"

namespace current_thread {
// __thread: TLS线程局部存储 每个当前线程都有一个该变量的实例
extern __thread int tls_thread_id;              //线程id
extern __thread const char* tls_thread_name;    //线程名字

//得到线程id syscall 
inline int thread_id() {
    if (__builtin_expect(tls_thread_id == 0, 0)) {
        tls_thread_id = static_cast<pid_t>(::syscall(SYS_gettid));
    }

    return tls_thread_id;
}

inline const char* thread_name() {
    return tls_thread_name;
}

}  // namespace thread_local_storage

namespace thread {
class Thread : utility::NonCopyAble {
 public:
    typedef std::function<void()> Worker;
    
    explicit Thread(const Worker&, const std::string& thread_name = "");
    ~Thread();

    //开始线程
    void Start();
    //join线程
    int Join();

    bool is_started() const {
        return is_started_;
    }

    pid_t thread_id() const {
        return thread_id_;
    }

    const std::string& thread_name() const {
        return thread_name_;
    }

 private:
    static void* Run(void* arg);
    void Run();

 private:
    Worker worker_;             //线程函数
    pthread_t pthread_id_;      //创建线程的id
    pid_t thread_id_;           //线程id
    std::string thread_name_;   //线程名字

    bool is_started_;    //线程是否已经开始
    bool is_joined_;     //线程是否已经join
};

}  // namespace thread

#endif  // THREAD_THREAD_H_
