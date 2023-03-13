// spin锁，如果没拿到锁就不断循环直到可以拿到
#include "spinlock.h"

// 因为一个进程中可能持有多把锁
// 而我们需要在第一把锁被acquire的时候就禁用中断
// 直到最后一把锁被release，才能重新开启中断
// 于是需要维护一个“中断acquire计数器”来实现上述目标
int lock_layers[8];

// 目的是初始化计数器
void spinlock_init(){
    for(int i = 0; i < 8; i ++){
        lock_layers[i] = 0;
    }
}

void push_inter_off(){
    int layer = lock_layers[read_tp()];
    if(layer == 0){
        // 仅在第一次上锁
        write_sstatus(read_sstatus() | ~SSTATUS_SIE);
    }
    layer ++;
    lock_layers[read_tp] = layer;
}

void pop_inter_off(){
    int layer = lock_layers[read_tp()];
    layer --;
    if(layer == 0){
        // 所有的锁已经清除，可以重新开启中断
        write_sstatus(read_sstatus() | SSTATUS_SIE);
    }
}

void acquire(struct spinlock * lock){

    // 尽量不要在使用锁的期间引入中断
    // 在中断中如果用不同顺序acquire了锁，就有可能导致死锁
    // 出于以上考量，在acquire之后就禁用这个core的中断
    write_sstatus(read_sstatus() | ~SSTATUS_SIE);

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