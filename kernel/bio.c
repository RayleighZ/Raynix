include "type.h"
// 文件系统的buffer cache层
// 其主要职责如下
// 1、缓存一定额的磁盘sector于内存之中，加速其访问速度
// 2、保证对于sector的访问不存在racing

// “缓存”的数据结构
struct buf{
    int valid; // data字段中是否已经填充了来自sector的字段
    int handled2disk; // buffer的内容是否已经被递交给了disk
    uint block_no; // buffer对应的block number
    struct sleeplock splk;
    uint dev; // 磁盘设备号
    uchar data[1024]; // 真正缓存的数据

    uint link_no; // 对此buf的引用个数

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

// 获得指定dev指定block的buf
// 仅仅提供buf，不提供磁盘io
static struct buf * bget(uint dev, uint block_no){
    // 先试图获得block
    acquire(& cache.lock);

    // 校验此block是否已经被缓存
    struct buf * cursor;
    cursor = cache.head.next;
    while(cursor != &cache.head){
        if(cursor -> dev == dev && cursor -> block_no == block_no){
            // 当前buf就是目标buf
            // 递增引用个数
            cursor -> link_no ++;
            // 要对buf进行引用，所以需要获取sleep lock
            acquire_sleep(& cursor -> sllk);
            release(& cache -> splk);
            return cursor;
        }
        cursor = cursor.next;
    }

    // 执行至此，证明并没有buf的缓存
    // 需要挑选一个link次数最少的

    cursor = cache.head.prior;
    while(cursor != cache.head){
        // 尚未有引用，可以直接扬了缓存
        if(cursor -> link_no == 0){
            cursor -> dev = dev;
            cursor -> block_no = block_no;
            cursor -> valid = 0;
            cursor -> link_no ++;
            acquire_sleep(& cursor -> sllk);
            release(& cache -> splk);
            return cursor;
        }
    }
}

struct buf * bread(uint dev, uint block_no){
    struct buf * buffer = bget(dev, block_no); // 在bget之中已经获取了sleep lock
    if(!buffer -> valid){ // 证明当前buf尚未存入磁盘的信息
        virtio_disk_rw(buffer, 0);
        buffer -> valid = 1;
    }
    return b;
}

void bwrite(struct buf * buffer){
    // 理论上现在是拥有了sleep锁的，可以直接写入
    virtio_disk_rw(buffer, 1);
}

void brelease(struct buf * buffer){
    // 释放sleep锁
    release_sleep(& buffer -> sllk);
    // 对chache数组进行操作，需要获得自旋锁
    acquire(& cache -> splk);
    buffer -> link_no --;
    // 如果buffer的link_no归零，可以将buffer移动到最前面
    // 这样的话会方便后续valid查询
    if(b -> link_no == 0){
        // 插到双向链表头的后面

        // 维持移除buffer之后的剩余连接关系
        buffer -> prior -> next = buffer -> next;
        buffer -> next -> prior = buffer -> prior;

        buffer -> next = cache.head.next;
        buffer -> prior = & cache.head;
        cache.head -> next = buffer;
    }

    release(& cache -> splk);
}

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