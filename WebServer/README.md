# HttpServer V1.0总结

## 目前的技术要点
* 参照muduo，使用双缓冲技术实现了Log系统
* 使用小根堆+unordered_map实现定时器队列，在此基础上进一步实现了长连接的处理
* 使用RAII机制封装锁，让线程更安全
* 采用Reactor模式+EPOLL(ET)非阻塞IO
* 使用基于状态机的HTTP请求解析，较为优雅
* 使用了智能指针、bind、function、右值引用等一些c++11的新特性

## 待优化的地方
* 加入缓存机制(LRU/LFU)
* 优雅的关闭连接
* 目前仅支持处理`GET`和`HEAD`方法，其他方法待加入
* 错误请求的处理暂时只按照`400 Bad Request`返回（能区分错误但未能返回正确的错误码）
* 扩展为FileServer

## HTTPServer的Reactor模型
<div align=center><img src="https://s1.328888.xyz/2022/04/18/rhcx1.png"></div>
HttpServer使用主从Reactor多线程模型
* 主要工作流程
    1. Reactor主线程MainReactor对象通过Select/Epoll监控建立连接事件，收到事件后通过Acceptor接收，处理建立连接事件（目前只有一个事件：请求连接）
    2. Acceptor处理建立连接事件后，MainReactor将连接分配Reactor子线程给 SubReactor进行处理
    3. SubReactor将连接加入连接队列进行监听，并创建一个Epoll用于监听各种连接事件
    4. 当有新的事件发生时，SubReactor会调用连接对应ActiveChannel的Handler进行响应
* 优点
    1. 父线程与子线程的数据交互简单职责明确，父线程只需要接收新连接，子线程完成后续的业务处理
    2. 父线程与子线程的数据交互简单，Reactor 主线程只需要把新连接传给子线程，子线程无需返回数据

## Reactor核心实现
### TimerManager
---
首先从定时器说起，每个定时器（`TimerNode`）管理一个Channel,然后由一个TimerManager来管理多个TimerNode。TimerManager通过一个小根堆（`timerHeap_`）和一个哈希表（`timerMap_`）来管理，`timerMap_`是fd对TimerNode的映射，映射Channel中最接近超时的TimerNode（每个Channel对应多个TimerNode），仅当Channel对应的所有TimerNode都被删除，才调用Channel的close回调函数，并将其从`timerMap_`中移除，符合长连接的处理逻辑
* 使用小根堆的原因：
    1. 优先队列不支持随机访问
    2. 即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构
* 具体处理逻辑：
    1. 对于被置为deleted的时间节点，会延迟到它①超时或②它前面的节点都被删除时，它才会被删除
    2. 因此一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除
* 这样做的好处：
    1. 不需要遍历优先队列，省时
    2. 给超时的节点一个容忍时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，就不用再重新new一个Channel节点了，这样可以复用前面的Channel，减少了一次delete和一次new的时间
* 待优化
    * 在同一长连接反复请求时会产生大量的TimerNode，可能会出现内存泄漏的情况

### Epoll
---
<div align=center><img src="https://s1.328888.xyz/2022/04/18/rhlO3.png"></div>
Epoll类是对系统调用epoll的封装,用一个就绪事件列表`std::vector<epoll_event> events_`接收`epoll_wait()`返回的活动事件列表,进一步通过`channelMap_`找到对应的channel(`epoll_event`的封装)并设为活动Channel
* 待优化
    * fd对channel的映射所造成的额外空间,**可以考虑优化为直接使用`epoll_event`的内置`data`**


### Channel
---
Channel是对epoll_event的封装,并将可能产生事件的文件描述符封装在其中的，这里的文件描述符可以是file descriptor，可以是socket，还可以是timefd，signalfd
* 每个Channel对象自始至终只属于一个EventLoop，因此每个Channel对象都只属于某一个IO线程
* 每个Channel对象自始至终只负责一个文件描述符（fd）的IO事件分发，在其析构的时候关闭这个fd，事件分发的基本思想是根据Epoll返回的事件活动flags，来进行回调函数的调用
* Channel的生命期由其owner calss负责管理

### EventLoop
---
EventLoop是事件驱动模式的核心,核心是为线程提供运行循环，不断监听事件、处理事件，符合muduo中one loop per thread的思想，执行提供在loop循环中运行的接口,它管理一个`Epoll`、一个用于loop线程间通信的`pwakeupChannel_`以及一个定时器队列`timerManager_`
* pwakeupChannel_使用eventfd来进行线程(eventloop)间的通信
* EventLoop在Loop()中主要做以下几件事：
    1. 清空`activechannels`
    2. 获取`activechannels`
    3. 根据事件处理`activechannels`中的活动`Channel`
    4. 处理`timerManager_`中超时的`TimerNode`
* 待优化
    * 将`doPendingFunctors()`作为`pwakeupChannel_`的ReadHandler，会造成惊群现状，目前没想到好的解决办法

## 线程池模型
<div align=center><img src="https://s1.328888.xyz/2022/04/18/rhKNO.png"></div>

借鉴muduo中one loop per thread的思想，每个线程与一个EventLoop对应，设计了`EventLoopThread`类封装了thread和EventLoop，然后在`EventLoopThreadPool`类中根据需要的线程数来创建`EventLoopThread`，最后根据Round Robin来选择下一个EventLoop，实现负载均衡

## 日志系统的实现
LOG的实现参照了muduo，但是比muduo要简化一点
* 首先是Logger类，Logger类里面有Impl类，其实具体实现是Impl类，我也不懂muduo为何要再封装一层，那么我们来说说Impl干了什么，在初始化的时候Impl会把时间信息存到LogStream的缓冲区里，在我们实际用Log的时候，实际写入的缓冲区也是LogStream，在析构的时候Impl会把当前文件和行数等信息写入到LogStream，再把LogStream里的内容写到AsyncLogging的缓冲区中，当然这时候我们要先开启一个后端线程用于把缓冲区的信息写到文件里
* LogStream类，里面其实就一个Buffer缓冲区，是用来暂时存放我们写入的信息的.还有就是重载运算符，因为我们采用的是C++的流式风格
* AsyncLogging类，最核心的部分，在多线程程序中写Log无非就是前端往后端写，后端往硬盘写，首先将LogStream的内容写到了AsyncLogging缓冲区里，也就是前端往后端写，这个过程通过append函数实现，后端实现通过threadfunc函数，两个线程的同步和等待通过互斥锁和条件变量来实现，具体实现使用了双缓冲技术
    * 双缓冲技术的基本思路：准备两块buffer，A和B,前端往A写数据，后端从B里面往硬盘写数据，当A写满后，交换A和B，如此反复。不过实际的实现的话和这个还是有点区别，具体看代码吧
* 待优化
    * 加入日志分级分文件存放
    * 加入滚动日志功能（按照行数和事件滚动）


# HttpServer V2.0总结

## 新加入的技术要点

* 结合哈希桶(仿照STL allocator)和单级链表实现线程安全的内存池，进一步减少内存碎片
* 采用双级链表和哈希表实现线程安全的LFU，在此基础上实现了服务器的页面缓存

## 待扩展的地方

* 扩展为FileServer（V1.0就说要加的...）
* 加入对SSL协议的支持

## 内存池

### 一些问题

* 为什么要使用内存池？

> 1. 使用`malloc/new`申请分配堆内存时系统需要根据最先匹配、最优匹配或其它算法在内存空闲块表中查找一块空闲内存；使用`free/delete`释放堆内存时，系统可能需要合并空闲内存块，会产生额外开销
> 2. 频繁使用时会产生大量内存碎片，从而降低程序运行效率

* 线程池的优缺点？
> 1. 优点：
> * 提高了内存的使用频率，每次的分配和释放并不是通过系统调用`malloc/new`和`free/delete`去操作实际的内存，而是在复用内存池中的内存
> * 减少内存碎片的产生，创建内存池中分配的都是一块块比较规整的内存块，减少内存碎片的产生
> 2. 缺点：
> * 会造成内存的浪费，因为要使用内存池需要在一开始分配一大块闲置的内存，而这些内存不一定全部被用到

* 不定长内存池和定长内存池的区别？
> 1. 不定长内存池
> * 不需要为不同的数据类型创建不同的内存池，其缺点是无法将分配出的内存回收到池内
> * 典型实现：`Apache Portable Runtime`中的`apr_pool`和`GNU lib C`中的`obstack`
> 2. 定长内存池
> * 在使用完毕后，将内存归还到内存池中，但需要为不同类型的数据结构创建不同的内存池，需要内存的时候要从相应的内存池中申请内存
> * 典型实现：`boost_pool`

### 具体实现

内存池中哈希桶的思想借鉴了STL allocator，具体每个内存池的实现参照了GitHub上的项目[An easy to use and efficient memory pool allocator written in C++.](https://github.com/cacay/MemoryPool)

<div align=center><img src="https://s1.328888.xyz/2022/04/28/A9VsZ.jpg"></div>

主要框架如上图所示，主要就是维护一个哈希桶`MemoryPools`，里面每项对应一个内存池`MemoryPool`，哈希桶中每个内存池的块大小`BlockSize`是相同的（4096字节，当然也可以设置为不同的），但是每个内存池里每个块分割的大小（槽大小）`SlotSize`是不同的，依次为8,16,32,...,512字节（需要的内存超过512字节就用`new/malloc`），这样设置的好处是可以保证内存碎片在可控范围内

<div align=center><img src="https://s1.328888.xyz/2022/04/28/A9oHO.jpg"></div>

内存池的内部结构如上图所示，主要的对象有：指向第一个可用内存块的指针`Slot* currentBlock`（也是图中的`ptr to firstBlock`，图片已经上传懒得改了），被释放对象的slot链表`Slot* freeSlot`，未使用的slot链表`Slot* currentSlot`，下面讲下具体的作用：

* `Slot* currentBlock`：内存池实际上是一个一个的 `Block` 以链表的形式连接起来，每一个 `Block` 是一块大的内存，当内存池的内存不足的时候，就会向操作系统申请新的 `block` 加入链表。

* `Slot* freeSlot`：链表里面的每一项都是对象被释放后归还给内存池的空间，内存池刚创建时 `freeSlot` 是空的。用户创建对象，将对象释放时，把内存归还给内存池，此时内存池不会将内存归还给系统（`delete/free`），而是把指向这个对象的内存的指针加到 `freeSlot` 链表的前面（前插），之后用户每次申请内存时，`memoryPool`就先在这个 `freeSlot` 链表里面找。

* `Slot* currentSlot`：用户在创建对象的时候，先检查 `freeSlot` 是否为空，不为空的时候直接取出一项作为分配出的空间。否则就在当前 `Block` 将 `currentSlot` 所指的内存分配出去，如果 `Block` 里面的内存已经使用完，就向操作系统申请一个新的 `Block`。

### 线程安全

内存池本身是线程安全的：在分配和删除 `Block` 和获取 `Slot` 时加锁 `MutexLockGuard lock(mutex_);`

### 缺陷

* 内存池工作期间的内存只会增长，不释放给操作系统。直到内存池销毁的时候，才把所有的 `block` 释放掉。这样在经过短时间大量请求后，会导致程序占用大量内存，然而这内存大概率是不会再用的（除非再有短时间的大量请求）。

### 一些可以优化的地方

*  `MemoryPool` 中使用了额外空间（一个链表）来维护 `freed slots` ，实际上这部分是可以优化的：将 `struct Slot` 改为 `union Slot` ，里面存放 `Slot* next` 和 `T value` ，这样可以保证每个 `Slot` 在没被分配/被释放时，在内存中就包含了指向下个可用 `Slot` 的指针，对于已经分配了的 `Slot` ，就存放用户申请的值。这样可以避免建立链表的开销（这个做法忘记在哪见过了，好像是STL）

* 针对前面所说的缺陷，我觉得可以在服务器空闲（怎么判断呢？）时，将空闲内存释放（释放多少呢？释放哪部分呢？还可以结合页面置换算法？），归还给操作系统

## LFU

### 一些问题

* 为什么要使用LFU而不是LRU？
> 1. LRU：最近最少使用，发生淘汰的时候，淘汰访问时间最旧的页面。LFU：最近不经常使用，发生淘汰的时候，淘汰频次低的页面。
> 2. 对于时间相关度较低（页面访问完全是随机，与时间无关） `WebServer` 来说，经常被访问的页面在下一次有更大的可能被访问，此时使用LFU更好，并且LFU能够避免周期性或者偶发性的操作导致缓存命中率下降的问题；而对于时间相关度较高（某些页面在特定时间段访问量较大，而在整体来看频率较低）的 `WebServer` 来说，在特定的时间段内，最近访问的页面在下一次有更大的可能被访问，此时使用LRU更好。
> 3. 目前设计的 `WebServer` 时间相关度较低，所以选择LFU。

### 具体实现

LFU的原理比较简单，实现方法也比较多样（leetcode上面就有设计LFU和LRU的题），我目前采用的是双重链表+hash表的经典做法。

<div align=center><img src="https://s1.328888.xyz/2022/04/28/AJAJQ.jpg"></div>

LFU的主要框架如上图所示，主要对象有四个：一个大链表 `FreqList` ，一个小链表 `KeyList`，一个 `key->FreqList` 的哈希表 `FreqHash` 以及一个 `key->KeyNode` 的哈希表 `KeyNodeHash` ，下面简单说下作用：

* `FreqList`：链表的每个节点都是一个小链表附带一个值表示频度，频度用来记录每个key的访问次数

* `KeyList` ：同一频度下的key value节点 `KeyNode`，只要找到 `key` 对应的 `KeyNode` ，就可以找到对应的 `value`

* `FreqHash` ：key到大链表节点的映射，这个主要是为了更新频度，因为更新频度需要先将 `KeyNode` 节点从当前频度的小链表中删除，然后加入下一频度的小链表（通过遍历大链表即可找到）

* `KeyNodeHash`：key到小链表节点的映射，这个Hash主要是为了根据key来找到对应的 `KeyNode` ，从而获得对应的value

* LRU中主要就两个操作： `bool LFUCache::get(string& key, string& val)` 和 `void LFUCache::set(string& key, string& val)` 。`get(2)` 就是先在 `FreqHash` 中找有没有 `key` 对应的节点，如果没有，就是缓存未命中，由用户调用 `set(2)` 向缓存中添加节点，如果缓存已满的话就在频度最低的小链表中删除最后一个节点；如果有，就是缓存命中，此时需要更新当前节点的频度（先将 `KeyNode` 节点从当前频度的小链表中删除，然后加入下一频度的小链表），加入下一频度的小链表的头部，这部分与LRU类似。

* 而对于 `WebServer` 来说，key就是对应的文件名，value就是对应的文件内容（通过 `mmap()` 映射），这样来建立页面缓存还是比较简单的。
 
### 线程安全

LFU本身是线程安全的：这部分的线程安全暂时没有好的想法，简单的在 `get(2)` 和 `set(2)` 加了互斥锁 `MutexLockGuard lock(mutex_);`

### 缺陷

* 由于LFU的淘汰策略是淘汰访问次数最小的数据块，但是新插入的数据块的访问次数为1，这样就会产生缓存污染，使得新数据块被淘汰。换句话说就是，最近加入的数据因为起始的频率很低，容易被淘汰，而早期的热点数据会一直占据缓存。

* 对热点数据的访问会导致 `freq_` 一直递增，我目前使用 `int` 表示 `freq_` ，实际上会有溢出的风险。

### 一些可以优化的地方

* 针对上文的缺陷,目前有三个方向改进：

1. [LFU-Aging](https://blog.csdn.net/yunshuipiao123/article/details/98958750)：在LFU算法之上，引入访问次数平均值概念，当平均值大于最大平均值限制时，将所有节点的访问次数减去最大平均值限制的一半或一个固定值。相当于热点数据“老化”了，这样可以避免频度溢出，也能缓解缓存污染

2. window-LFU：Window是用来描述算法保存的历史请求数量的窗口大小的。Window-LFU并不记录所有数据的访问历史，而只是记录过去一段时间内的访问历史。即当请求次数达到或超过window的大小后，每次有一条新的请求到来，都会清理掉最旧的一条请求，以保证算法记录的请求次数不超过window的大小。

3. redis-LFU：redis的实现结合了LRU和LFU，通过定时器，每n秒淘汰一次过期缓存（LRU），过期缓存不会马上被删除，而是加入一个过期队列，然后真正需要淘汰时，从过期队列中删除使用频率最少的键（LFU）。redis不太了解，讲的是自己的理解


