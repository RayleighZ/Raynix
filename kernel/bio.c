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
    struct
};

void binit(){

}