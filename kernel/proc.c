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

// 在获取当前cpu的proc时如果触发了时钟中断
// 则将引起当前cpu的proc变化，使得先前获取的proc不再正确
// 所以这里需要进行一次封装，在获取proc前后禁用中断
// 并且更加合理的做法应该是向拿锁一样，push以及pop中断
// 原因与spinlock处相同
struct * cpu cur_proc(){
    push_inter_off();
    int id = read_tp();
    struct proc * p = cpus[id] -> cur_proc;
    pop_inter_off();
    return p;
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
    struct proc * p = cur_proc();
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
    struct proc * p = cur_proc();
    acquire(&p->lock);
    // 因为被放弃了，就设置状态为可运行（但尚未运行）
    p -> state = RUNNABLE;
    enter_sched();
    // 解锁，这里的锁是在scheduler之中加上的
    release(&p->lock);

    // 上文中这种交叉式的锁的使用保证了对其中一个cpu对proc寄存器的使用都是单独的
    // 不会导致多核跑在同一个proc信息（特别是stack）上的窘境
    // 可以保证在swtch前后，一直有对proc的保护
}

void channel_push(int channel, struct proc * process){
    struct sleeping_chain head = chains[channel];
    head -> process;
}

void wakeup(int channel){
    struct proc cursor;
    for(int i = 0; i < 64; i ++){
        cursor = procs[i];
        if(cursor != cur_proc()){
            // 获取p的lock，尝试修改状态
            acquire(cursor -> lock);
            if(cursor -> channel == channel 
            && cursor -> state == SLEEPING){
                // 修改状态将其激活
                // 因为存在schedule机制，可以认为修改到runnable之后立刻就会被执行
                cursor -> state = RUNNABLE;
            }
            release(cursor -> lock);
        }
    }
}

// 让当前进程yield，直到其他进程将其wake up
void sleep(int channel, struct spinlock * lock){
    // 获取当前进程
    struct proc * p = cur_proc();
    // 先要获取p->lock，因为这里在修改进程状态
    acquire(&p->lock);
    // 修改p的状态
    p -> state = SLEEPING;
    p -> channel = channel;
    chains[channel]
    // 在进入调度器之前需要release lock，防止死锁
    release(lock);
    enter_sched();

    // 执行到这里时，意味着当前进程已经
    // 放弃p->lock，理由同give_up
    // 同时需要清理sleeping channel
    p -> channel = 0;
    release(p->lock);
    // 重新获得lock，因为sleep往往涉及一些整体状态的修改
    // 调用sleep的进程应该希望在sleep之后lock还在
    acquire(lock);
}