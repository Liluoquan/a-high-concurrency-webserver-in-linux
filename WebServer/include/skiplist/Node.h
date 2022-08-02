#pragma once

template <typename K, typename V>
class SkipList;

template <typename K, typename V>
class Node {
    friend class SkipList<K, V>;
public:
    Node() {}
    Node(K k, V v, int level);
    ~Node();

    K getKey() const;
    V getValue() const;
    void setValue(V v);

    // 不同层数的下一节点
    Node<K, V> **_forward;
    int _nodeLevel;

private:
    K _key;
    V _value;
};

template <typename K, typename V>
Node <K, V>::Node(const K k, const V v, int level) {
    _key = k;
    _value = v;
    _nodeLevel = level;

    _forward = new Node<K, V> *[_nodeLevel + 1] ();
}

template <typename K, typename V>
Node<K, V>::~Node() {
    delete[] _forward;
}

template <typename K, typename V>
K Node<K, V>::getKey() const {
    return _key;
}

template <typename K, typename V>
V Node<K, V>::getValue() const {
    return _value;
}

template <typename K, typename V>
void Node<K, V>::setValue(V v) {
    _value = v;
}


