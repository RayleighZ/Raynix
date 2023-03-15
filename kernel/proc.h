// 进程、trampoline、trapframe相关结构的定义

// trapframe的定义（来自xv6）
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};


// 进程调度中缓存寄存器使用的context
struct context {
    uint64 ra;
    uint64 sp;
    // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
}

static int RUNNING = 0;
static int RUNNABLE = 1;

struct proc{
    struct trapframe * tf; // trapframe，中断时存储寄存器用
    struct context context; // 进程调度时缓存寄存器用
    struct spinlock lock;
    int state; // 当前process的运行状态
};

// cpu的定义
struct cpu{
    // 是否开启中断
    // 在spinlock中用于校验是否需要在lock层数清零之后开启中断
    int interrupt_disabled;
    struct proc * cur_proc; // 当前执行的proc
    int context; // 保存scheduler的context，在proc.c的scheduler中被加载
    int lock_layer; // 当前加的锁的层数，用于在spinlock中判断是否需要开中断
    pagetable_t pagetable;
}

extern struct cpu cpus[8];