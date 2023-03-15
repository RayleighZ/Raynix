// 进程、trampoline、trapframe相关结构的定义

struct proc{

};

// cpu的定义
struct cpu{
    // 是否开启中断
    // 在spinlock中用于校验是否需要在lock层数清零之后开启中断
    int interrupt_disabled;
    struct proc * cur_proc; // 当前执行的proc
    int context; // 保存scheduler的context，在proc.c的scheduler中被加载
    int lock_layer; // 当前加的锁的层数，用于在spinlock中判断是否需要开中断
}

extern struct cpu cpus[8];