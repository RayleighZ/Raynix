#include "type.h"
#include "risc_v.h"
#include "proc.h"
#include "def.h"
// 进程数组（懒加载）
struct proc procs[64];

// 为每一个进程分配kernel stack
// 值得注意的是，这里采用了懒加载
// 所支持的最大进程数是64，故在这里一次性分配64个kernel stack
void init_proc_kstack(pagetable_t pgt){
    for(int i = 0; i < 64; i++){
        // 为KSTACK分配一页内存
        char * pa = kalloc();
        // KSTACK的位置在trampoline之下，间隔一个proc安置一次
        int offset = int (&proc[i] - proc);
        uint64 va = KSTACK(offset);
        map(pgt, va, (uint64)pa, 4096, PTE_R | PTE_W);
    }
}

// 每一个cpu都应该有一个kernel thread运行此
// 用于找到下一个可以切换时间片的process
void scheduler(){

    struct cpu * c = cpus[read_tp()];
    struct proc * cusor;
    c -> cur_proc = 0;
    for(;;){
        // 循环查找所有的process中runnable的
        for(int i = 0; i < 64, i++){
            cusor = &procs[i];
            // 校验是否runnable
            acquire(cusor -> lock);
            if(cusor -> state == RUNNABLE){
                // 尝试拿锁
                // 拿到之后就意味着可以切换了
                // 但是需要先更新当前cpu的状态
                cusor -> state = RUNNING;
                c -> cur_proc = cusor;
                swtch(c -> context, cusor -> context);

                // 如果再回到这里，意味着cursor被yield了
                // 当前cpu不再有正在run的proc，故cur_proc需要清零
                c -> cur_proc = 0;
            }
            release(cusor -> lock);
        }
    }
}

// 切换当前进程进入scheduler
void enter_sched(){
    // 如果持有锁然后放弃cpu将可能导致其他程序长时间拿不到锁
    // 因此在放弃之前需要校验锁的层数是否为1（cpu持有p->lock是可接受的）
    struct cpu c = cpus[read_tp()];
    struct proc * p = c -> cur_proc;
    if(c -> lock_layer == 1){
        // 是否开启中断操纵的是cpu，但本质上其实是process的属性
        // 如果我们开启swtch，当再回来的时候，cpu->interrupt_disabled其实已经被另一个进程刷新
        // 所以需要缓存当前process的interrupt_disabled状态
        // 当下次swtch回来的时候再恢复缓存
        int disabled = c -> interrupt_disabled;
        swtch(p -> context, c -> context);
        c -> interrupt_disabled = disabled;
    } else {
        // TODO: panic
    }
}


// 让当前进程放手，切入到scheduler
void give_up(){
    // 拿锁，这个锁会在scheduler之中解开
    struct proc * p = cpus[read_tp()] -> cur_proc;
    acquire(&p->lock);
    // 因为被放弃了，就设置状态为可运行（但尚未运行）
    p -> state = RUNNABLE;
    enter_sched();
    // 解锁，这里的锁是在scheduler之中加上的
    release(&p->lock);
}