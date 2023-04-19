include "type.h"
// 文件系统的buffer cache层
// 其主要职责如下
// 1、缓存一定额的磁盘sector于内存之中，加速其访问速度
// 2、保证对于sector的访问不存在racing

// “缓存”的数据结构
struct buf{
    int vaild; // data字段中是否已经填充了来自sector的字段
    int handled2disk; // buffer的内容是否已经被递交给了disk
    uint block; // buffer对应的block number
    struct sleeplock splk;

    uchar data[1024]; // 真正缓存的数据
    
    // buf使用双向链表存储
    struct buf * prior;
    struct buf * next;
};

// buffer cache层所有缓存buffer数组
struct buf buffers[30];

// 存储着buffer双向链表的结构体变量
struct {
    struct buf head;
    struct spinlock lock;
} cache;

void binit(){
    // 初始化锁
    spinlock_init(& cache.lock);

    // 初始化双向链表
    struct buf * cursor;
    cursor = & buffers[0];
    cache.head -> next = cursor;
    cursor -> prior = cache;
    sleeplock_init(& cursor -> splk);
    for(int i = 1; i < 30; i ++){
        cursor -> next = & buffers[i];
        buffers[i].prior = cursor;
        // 为每一个buf加载一份sleeplock
        sleeplock_init(& cursor -> splk);
        cursor = & buffers[i];
    }
    cursor -> next = & cache.head;
    cache.head -> prior = cursor;
}