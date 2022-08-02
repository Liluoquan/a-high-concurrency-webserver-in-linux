#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <iostream>
#include "Node.h"

// #define DEBUG

const static std::string STORE_FILE =  "../store/dumpFile";
const static std::string DELIMITER = ":";

static std::mutex mtx;

template <typename K, typename V>
class SkipList {
public:
    SkipList();
    SkipList(int maxLevel);
    ~SkipList();

    Node<K, V>* createNode(K, V, int);
    void displayList();
    int insertElement(const K, const V);
    bool searchElement(K, V&);
    bool deleteElement(K);

    int size();
    
    void dumpFile();
    void loadFile();

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);
    int getRandomLevel();

private:
    // 跳表的最大层数
    int _maxLevel;
    
    // 当前所在的层数
    int _curLevel;

    // 跳表头节点指针
    Node<K, V>* _header;

    // 当前元素个数
    int _elementCount;

    // 文件操作
    std::ofstream _fileWriter;
    std::ifstream _fileRader;

#ifdef RANDOM
    Random rnd;
#endif
};

// 创建节点
template <typename K, typename V>
Node<K, V>* SkipList<K, V>::createNode(const K k, const V v, int level) {
    Node<K, V>* node = new Node<K, V>(k, v, level);
    return node;
}

template <typename K, typename V>
SkipList<K, V>::SkipList() {
    _maxLevel = 32;
    _curLevel = 0;
    _elementCount = 0;

    // 创建头节点，并将K与V初始化为NULL
    K k;
    V v;
    _header = new Node<K, V>(k, v, _maxLevel);
    loadFile();
}

// 建造跳表
template <typename K, typename V>
SkipList<K, V>::SkipList(int maxLevel) {
    _maxLevel = maxLevel;
    _curLevel = 0;
    _elementCount = 0;

    // 创建头节点，并将K与V初始化为NULL
    K k;
    V v;
    _header = new Node<K, V>(k, v, _maxLevel);
}

// 析构
template <typename K, typename V>
SkipList<K, V>::~SkipList() {
    if(_fileRader.is_open()) {
        _fileRader.close();
    }
    if(_fileWriter.is_open()) {
        _fileWriter.close();
    }

    delete _header;
}

// 向跳表中插入节点
// Insert given key and value in skip list
// return 1 means element exists  
// return 0 means insert successfully

/*
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+

*/

template <typename K, typename V>
int SkipList<K, V>::insertElement(const K key, const V value) {
    mtx.lock();
    Node<K, V>* cur = _header;

    // 创建数组update并初始化该数组
    Node<K, V>** update = new Node<K, V>* [_maxLevel + 1]();
    
    // 从跳表的左上角节点开始查找
    for(int i = _curLevel; i >= 0; --i) {
        // cur在该层的下一个节点的 _key 小于要找的 key
        while(cur->_forward[i] != nullptr && cur->_forward[i]->getKey() < key) {
            cur = cur->_forward[i];
        }
        update[i] = cur;
    }

    // 到达最底层（第0层）,并且当前的 forward 指针指向第一个大于待插入节点的节点
    cur = cur->_forward[0];

    // 如果当前节点的 key 值和待插入节点 key 相等，说明节点已经存在，修改节点值即可
    if(cur != nullptr && cur->getKey() == key) {
#ifdef DEBUG
        std::cout << "key: " << key << ", exists. Change it" << std::endl;
#endif
        // FIXME:do we need to change it?
        cur->setValue(value);
        mtx.unlock();
        return 1;
    }

    //如果current节点为null 这就意味着要将该元素插入最后一个节点。
    //如果current的key值和待插入的key不等，代表我们应该在update[0]和current之间插入该节点。
    if(cur == nullptr || cur->getKey() != key) {
        int randomLevel = getRandomLevel();

        if(randomLevel > _curLevel) {
            for(int i = _curLevel + 1; i <= randomLevel; ++i) {
                update[i] = _header;
            }
            _curLevel = randomLevel;
        }

        // 使用生成的random level创建新节点
        Node<K, V>* insertNode = createNode(key, value, randomLevel);

        // 插入节点
        for(int i = 0; i <= randomLevel; ++i) {
            insertNode->_forward[i] = update[i]->_forward[i];
            update[i]->_forward[i] = insertNode;
        }
#ifdef DEBUG
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
#endif
        ++_elementCount;
    }

    mtx.unlock();
    return 0;
}


// Search for element in skip list
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/
template <typename K, typename V>
bool SkipList<K, V>::searchElement(K key, V& value) {
#ifdef DEBUG
    std::cout << "search_element-------------" << std::endl;
#endif
    Node<K, V>* cur = _header;
    
    // 从跳表的最左上角开始查找
    for(int i = _curLevel; i >= 0; --i) {
        while(cur->_forward[i] && cur->_forward[i]->getKey() < key) {
            cur = cur->_forward[i];
        }
    }

    // 最底层的下一个，会出现两种情况
    // 1、就是需要找的 key
    // 2、比找的key 要大
    cur = cur->_forward[0];
    
    if(cur != nullptr && cur->getKey() == key) {
        value = cur->getValue();
#ifdef DEBUG
        std::cout << "Found key: " << key << ", value: " << cur->getValue() << std::endl;
#endif
        return true;
    }
#ifdef DEBUG
    std::cout << "Not Found Key:" << key << std::endl;
#endif

    return false;
}

template <typename K, typename V>
bool SkipList<K, V>::deleteElement(K key) {
    mtx.lock();

    Node<K, V>** update = new Node<K, V>* [_maxLevel + 1]();

    Node<K, V>* cur = _header;

    for(int i = _curLevel; i >= 0; --i) {
        while(cur->_forward[i] != nullptr && cur->_forward[i]->getKey() < key) {
            cur = cur->_forward[i];
        }
        update[i] = cur;
    }
    cur = cur->_forward[0];

    if(cur != nullptr && cur->getKey() == key) {
        for(int i = 0; i <= _curLevel; ++i) {
            if(update[i]->_forward[i] != cur) {
                break;
            }
            update[i]->_forward[i] = cur->_forward[i];
        }
        delete cur;

        while(_curLevel > 0 && _header->_forward[_curLevel] == nullptr) {
            --_curLevel;
        }
        --_elementCount;

        mtx.unlock();

        return true;
    }
    else {
        mtx.unlock();
        return false;
    }
}

// 打印整个跳表
template <typename K, typename V>
void SkipList<K, V>::displayList() {
#ifdef DEBUG
    std::cout << "display skipList : " << std::endl;
#endif

    Node<K, V>* cur;

    for(int i = _curLevel; i >= 0; --i) {
        cur = _header->_forward[i];
#ifdef DEBUG
        std::cout << "Level : " << i << ':' << std::endl;
#endif
        while (cur != nullptr) {
#ifdef DEBUG
            std::cout << cur->getKey() << ':' << cur->getValue() << ' ';
#endif
            cur = cur->_forward[i];
        }
#ifdef DEBUG
        std::cout << std::endl;
#endif
    }
    return;
}

#ifndef RANDOM
template <typename K, typename V>
int SkipList<K, V>::getRandomLevel() {
    int k = 0;
    while (rand() % 2) {
        ++k;
    }
    k = (k < _maxLevel) ? k : _maxLevel;
    return k;
}

#else
int SkipList<K, V>::getRandomLevel() {
    int level = static_cast<int>(rnd.Uniform(_maxLevel));
    while(rand() % 2) {
        ++k;
    }
    k = (k < _maxLevel) ? k : _maxLevel;
    return k;
}
#endif

template <typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    if (str.empty() || str.find(DELIMITER) == std::string::npos) {
        return false;
    }
    
    return true;
}

template <typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {
    if(!is_valid_string(str)) {
        return;
    }
    int pos = str.find(DELIMITER);
    *key = str.substr(0, pos);
    *value = str.substr(pos + 1, str.size());
}

// 将数据导出到文件
template <typename K, typename V>
void SkipList<K, V>::dumpFile() {
#ifdef DEBUG
    std::cout << "dumpFile ---------------- " << std::endl;
#endif

    if (!_fileWriter.is_open()) {
        _fileWriter.open(STORE_FILE, std::ios::out | std::ios::trunc);
    }

    // _fileWriter.open(STORE_FILE);
    Node<K, V>* cur = _header->_forward[0];

    while(cur != nullptr) {
        _fileWriter << cur->getKey() << ":" << cur->getValue() << "\n";

#ifdef DEBUG
        std::cout << cur->getKey() << ":" << cur->getValue() << ";\n";
#endif

        cur = cur->_forward[0];
    }

    _fileWriter.flush();
    _fileWriter.close();
    return;
}

// 从磁盘读取数据
template <typename K, typename V>
void SkipList<K, V>::loadFile() {
#ifdef DEBUG
    std::cout << "loadFile ---------------- " << std::endl;
#endif

    _fileRader.open(STORE_FILE);
    std::string line;
    // FIXME: why use new?
    std::string* key = new std::string();
    std::string* value = new std::string();
    while(getline(_fileRader, line)) {
        get_key_value_from_string(line, key, value);
        if(key->empty() || value->empty()) {
            continue;
        }
        insertElement(*key, *value);
#ifdef DEBUG
        std::cout << "key:" << *key << "value:" << *value << std::endl;
#endif

    }
    _fileRader.close();
}

// 返回跳表的元素个数
template <typename K, typename V>
int SkipList<K, V>::size() {
    return _elementCount;
}






