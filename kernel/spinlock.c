// spin锁，如果没拿到锁就不断循环直到可以拿到
#include "types.h"
#include "spinlock.h"
#include "memlayout.h"
#include "risc_v.h"

// 因为一个进程中可能持有多把锁
// 而我们需要在第一把锁被acquire的时候就禁用中断
// 直到最后一把锁被release，才能重新开启中断
// 于是需要维护一个“中断acquire计数器”来实现上述目标
// 同时，还需要维持一个是否在acquire之前就已经设置为中断关闭的数组
// 在acquire之前就已经将中断关闭，则就算锁层归零也不应该开启中断

int lock_layers[8]; // 锁的层数
int off_from_start[8]; // 一开始是否就关闭了中断

// 目的是初始化计数器
void spinlock_init(){
    for(int i = 0; i < 8; i ++){
        lock_layers[i] = 0;
        off_from_start[i] = 0;
    }
}

// 初始化锁，主要目的是将是否available设置为1
void reset_lock(struct spinlock *lock){
    lock -> available = 1;
}

void push_inter_off(){

    int id = read_tp();
    int layer = lock_layers[id];
    if((read_sstatus() & SSTATUS_SIE == 0) || layer == 0){
        // 标志着本来就没有开启中断
        // 所以将标志位设置为1
        // 将来即使层数归零，也不应该开启中断
        off_from_start[id] = 1;
    } else {
        // 当然，如果不是，就要恢复为0
        off_from_start[id] = 0;
    }
    if(layer == 0){
        // 仅在第一次上锁
        write_sstatus(read_sstatus() | ~SSTATUS_SIE);
    }
    layer ++;
    lock_layers[id] = layer;
}

void pop_inter_off(){
    int id = read_tp();
    lock_layers[id] --;
    int layer = lock_layers[id];
    if(layer == 0 && off_from_start[id] == 0){
        // 所有的锁已经清除，并且在一开始就没有启动中断
        // 可以重新开启中断
        write_sstatus(read_sstatus() | SSTATUS_SIE);
    }
}

// 索要lock
void acquire(struct spinlock * lock){

    // 尽量不要在使用锁的期间引入中断
    // 在中断中如果用不同顺序acquire了锁，就有可能导致死锁
    // 出于以上考量，在acquire之后就禁用这个core的中断
    push_inter_off();

    // 因两个core可能同时发现lock可用而同时介入
    // 所以lock是否available的校验需要保证原子性
    // 这里使用c的__sync_lock_test_and_set
    // 它会最终调用risc-v的amoswap，原子性的交换寄存器与内存地址的值
    // 返回值是旧的指针指向的值（被新的值所替代）

    // 尝试替换available为0（使之不可用）
    // 但是需要前提是旧值是available的（1）
    while(__sync_lock_test_and_set(&(lock->available), 0) == 0);

    // 执行到此处时，lock的available虽然还是0，但已经变成过一次1了
    // 值得注意的是，为了保证原子性，一定不可写作while(flag){ flag = check }的形式
}

// 释放lock
void release(struct spinlock * lock){

    // 将available写入此锁中
    __sync_lock_test_and_set(&(lock->available), 1);

    // 重新开启中断
    pop_inter_off();
}