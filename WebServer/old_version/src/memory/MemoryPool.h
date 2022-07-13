// @Author: Lawson
// @CreateDate: 2022/04/21
// @ModifiedDate: 2022/04/21

#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <stdarg.h>
#include <utility>
#include <functional>
#include "../base/MutexLock.h"

#define BlockSize 4096

struct Slot {
    Slot* next;
};

// template <typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();

    void init(int size);

    // 分配或收回一个元素的内存空间
    Slot* allocate();
    void deAllocate(Slot* p);
private:
    int slotSize_;          // 每个槽所占字节数

    Slot* currentBolck_;    // 内存块链表的头指针
    Slot* currentSlot_;     // 元素链表的头指针
    Slot* lastSlot_;        // 可存放元素的最后指针
    Slot* freeSlot_;        // 元素构造后释放掉的内存链表头指针

    MutexLock mutex_freeSlot_;
    MutexLock mutex_other_;

    size_t padPointer(char* p, size_t align);   // 计算对齐所需空间
    Slot* allocateBlock();      // 申请内存块放进内存池
    Slot* nofree_solve();
};

void init_MemoryPool();
void* use_Memory(size_t size);
void free_Memory(size_t size, void* p);

// template <typename T, size_t BlockSize>
MemoryPool& get_MemoryPool(int id);

template<typename T, typename... Args>
T* newElement(Args&&... args) {
    T* p;
    if((p = reinterpret_cast<T *>(use_Memory(sizeof(T)))) != nullptr)
        // new(p) T1(value);
        // placement new:在指针p所指向的内存空间创建一个T1类型的对象，类似与realloc
        // 把已有的空间当成一个缓冲区来使用，减少了分配空间所耗费的时间
        // 因为直接用new操作符分配内存的话，在堆中查找足够大的剩余空间速度是比较慢的
        new(p) T(std::forward<Args>(args)...); // 完美转发

    return p;
}

// 调用p的析构函数，然后将其总内存池中释放
template<typename T>
void deleteElement(T* p) {
    // printf("deleteElement...\n");
    if(p)
        p->~T();
    free_Memory(sizeof(T), reinterpret_cast<void *>(p));
    // printf("deleteElement success\n");
}