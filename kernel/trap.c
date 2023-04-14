#include "type.h"
#include "risc_v.h"
#include "def.h"
#include "proc.h"

static int SOFTWARE_INTER = 0;
static int TIMER_INTER = 1;
static int EXTERNAL_INTER = 2;
static int SYS_CALL = 3;

extern char trampoline[], uservec[], userret[];

void user_trap();

void trap_init(){
    write_stvec((uint64)ktraph);// 指向kernel trap
}

int get_inter_type(uint64 scause){
    if((scause & 0x8000000000000000L) && (scause & 0xff) == 9){
        return EXTERNAL_INTER;
    } else if (scause == 0x8000000000000001L){
        return TIMER_INTER;
    } else if (scause == 8){
        return SYS_CALL;
    } else {
        return -1;
    }
}

// 处理内核中断
void kernel_trap(){
    uint64 scause = read_scause();
    // 校验是否开启内核中断
    if(check_open_interrupts() == 0){
        int type = get_inter_type();
        switch (type){
            case TIMER_INTER : {
                // 时钟中断
                // 直接yield
                break;
            }
            case EXTERNAL_INTER : {
                // 外部设备中断
                break;
            }
            default : {
                // -1，不在可处理范围之内
                // TODO: panic
            }
        }
    }

    // 恢复与返回的逻辑在ktraph.S中处理
}

// 返回到用户态
void return_2_user(){
    struct cpu c = cpus[read_tp()];
    struct proc * myproc = c -> cur_proc;

    // 因为即将返回用户态，所以需要将stvec指向utraph
    // 但毕竟现在还是在kernel，为了逻辑上的正确就先暂时关闭中断
    write_sstatus(read_sstatus() | ~SSTATUS_SIE);

    // 如上文所述，设置stvec为utraph
    uint64 uhandler = TRAMPOLINE + (uservec - trampoline);
    write_stvec(uhandler);

    // 用户态中断的时候需要加载kernel的sp tp
    // 顾这里需要提前写入用于准备
    struct trapframe * tf = myproc -> tf;
    tf -> kernel_trap = read_satp(); // 写入kernel pagetable 
    tf -> kernel_sp = myproc -> kstack + 4096; // 此process的kernel stack位置
    tf -> kernel_trap = (uint64) user_trap;
    tf -> kernel_hartid = read_tp();

    // 恢复sepc为缓存的user space触发之处
    write_sepc(tf -> epc);

    // 加载原本用户态的分页内存页表位置
    uint64 mask = 8L << 60;
    uint64 page = (mask | (((uint64)myproc -> pagetable) >> 12));
    
    // 调用trampoline中的userret回到用户态
    // 先提取出userret函数
    uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);

    // 清空SSP位（同时也是设置为恢复到user mode）
    uint64 sstauts = read_sstatus();
    sstatus &= ~(3L << 11);
    // 开启用户态中断（此函数的最初是关闭了中断的）
    sstatus |= SSTATUS_SPIE;
    write_sstatus(sstatus);
    
    // 调用之，并将页表塞进去
    ((void (*)(uint64))trampoline_userret)(pagetable);
}

// 处理用户层中断
// 这是一种交换的思想
// 用户层程序需要直到kernel的stack、pagetable才能切换过来执行
// 所以在返回用户态之前，kernel需要提前将这些必备信息存储在trapframe之中
void user_trap(){
    // 目前已经处于内核态，如果发生中断，应该交由ktraph处理
    // 用户层使用utraph以及内核态使用ktraph需要手动处理
    // 因为二者都会回到supervisor mode的内核，使用的都是stvec寄存器
    // 所以需要手动在进入kernel的时候修改为ktraph
    // 在离开kernel的时候设置为utraph
    // 当然只有在usertrap中需要设置
    // 本就来自kernel的中断自然不用再设置回去
    // 最终效果就是在内核态发生的中断交由ktraph处理
    // 在用户态发生的中断交由utraph处理
    write_stvec((uint64)ktraph);

    // 在前置处理中，并没有进行pc的缓存，因为pc已经指向了handler中的指令
    // 真正触发了中断（当然可能是异常）的指令的位置在sepc寄存器之中
    // 所以在tramframe中存储的pc必须是sepc
    struct cpu c = cpus[read_tp()];
    struct proc * myproc = c -> cur_proc;

    // 这里缓存epc是十分有必要的
    // 如果在处理user trap的时候又陷入了kernel trap
    // sepc寄存器就将覆盖
    // 所以需要提前缓存，将来返回用户态的时候再恢复
    cur_proc -> tf -> epc = read_sepc();

    uint64 scause = read_scause();

    // 校验interrupt的原因
    switch(scause){
        case SYS_CALL : {
            // TODO: 后续执行system call
            break;
        }
        
        case TIMER_INTER : {
            // TODO: yield到scheduler，进行进程调度
            break;
        }
    }

    // 返回user
    // ktraph的返回函数在汇编中定义了
    // utraph则没有相关逻辑，需要手动在c语言中进行恢复与返回
    return_2_user();
}