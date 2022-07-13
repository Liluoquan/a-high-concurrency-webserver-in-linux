#pragma once

#include "../memory/MemoryPool.h"
#include "../base/MutexLock.h"
#include <unordered_map>
#include <string>

#define LFU_CAPACITY 10

using std::string;

// 链表的节点
template<typename T>
class Node {
public:
    void setPre(Node* p) { pre_ = p; }
    void setNext(Node* p) { next_ = p; }
    Node* getPre() { return pre_; }
    Node* getNext() { return next_; }
    T& getValue() { return value_; }

private:
    T value_;
    Node* pre_;
    Node* next_;
};

// 文件名->文件内容的映射
struct Key {
    string key_, value_;
};

typedef Node<Key>* key_node;

// 链表：由多个Node组成
class KeyList {
public:
    void init(int freq);
    void destory();
    int getFreq();
    void add(key_node& node);
    void del(key_node& node);
    bool isEmpty();
    key_node getLast();

private:
    int freq_;
    key_node Dummyhead_;
    key_node tail_;
};

typedef Node<KeyList>* freq_node;

/*
典型的双重链表+hash表实现
首先是一个大链表，链表的每个节点都是一个小链表附带一个值表示频度
小链表存的是同一频度下的key value节点。
hash表存key到大链表节点的映射(key,freq_node)和
key到小链表节点的映射(key,key_node).
*/

// LFU由多个链表组成
class LFUCache {
private:
    freq_node Dummyhead_;    // 大链表的头节点，里面每个节点都是小链表的头节点
    size_t capacity_;
    MutexLock mutex_;

    std::unordered_map<string, key_node> kmap_;  // key到keynode的映射
    std::unordered_map<string, freq_node> fmap_; // key到freqnode的映射

    void addFreq(key_node& nowk, freq_node& nowf);
    void del(freq_node& node);
    void init();

public:
    LFUCache(int capicity);
    ~LFUCache();

    bool get(string& key, string& value); // 通过key返回value并进行LFU操作
    void set(string& key, string& value); // 更新LFU缓存
    size_t getCapacity() const { return capacity_; }
};

LFUCache& getCache();






