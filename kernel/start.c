#include "type.h"
#include "risc_v.h"

void main();

// 时钟中断handler所使用的scratch
uint64 timer_scratch[8][5];

// 时钟中断handler的位置
extern void ttraph();

// 初始化时钟中断
void timer_inter_init(){
    // 设置时钟中断的delay时间（此处设定为十分之一秒）
    int delay = 1000000;

    // 当前core的id
    int core_id = read_mhartid();

    // mtime寄存器会随着一定频率增加而增加
    // 当mtime大于等于mtimecmp的时候将触发时钟中断
    // 为每一个核心单独设置mtimecmp，错开中断触发
    uint64 mtime_since_boot = *(uint64 *)CLINT_MTIME;// 自从启动以来累计的CLINT_MTIME
    
    // 设置CMP，当mtime逐渐追上mtimecmp的时候，将触发时钟中断
    // 当mtime超出最大值之后，会回旋回来，以此产生周期性中断
    *(uint64 *)CLINT_MTIMECMP(core_id) = mtime_since_boot + delay;
    
    // 加载scratch[]。为timer的handler做准备
    // 因为每一个核心都会来到此处，也就意味着只需要针对本核心的scratch数组进行初始化
    timer_scratch[core_id][3] = CLINT_MTIMECMP(core_id);
    timer_scratch[core_id][4] = delay;

    // 写入machine mode的scratch（仅用写入自己核心的就可）
    write_mscratch((uint64)timer_scratch[core_id]);

    // 设置时钟中断handler
    write_mtvec((uint64)ttraph);

    // xstatus寄存器有着x模式下的全局中断使能
    // xie寄存器则控制具体类型中断的使能（时钟中断、软中断、外部中断）

    // 开启machine mode中断
    // mie位在3 bit
    write_mstatus(read_mie() | (1L << 3));

    // 开启machine mode时钟中断
    write_mie(read_mie() | MIE_MTIE);
}

void start(){
    // 当前hart处于machine模式（通过trap来到这里）
    // 应当在完成初始配置后回到supervisor模式，并进行后续内核启动
    
    // 将mstatus寄存器中trap来源的权限状态设置为supervisor
    // 以便在machine模式从trap中返回时可以正确处于supervisor mode
    unsigned long mstatus = read_mstatus();
    mstatus &= !(3L << 11);//将mstatus的11 12位MPP清零
    mstatus |= (1L << 11);//将上一个状态设置为supervisor

    write_mstatus(mstatus);

    // 将mepc指向main函数，这样的话在结束trap后，将会回到main函数
    // main函数将执行内核服务启动相关内容
    write_mepc((uint64)main);

    // 暂不开启分页寻址
    write_satp(0);

    // 将cpuid写入线程指针，以便在回到supervisor_mode之后依然可以访问到
    write_tp(read_mhartid());
    
    // 从machine_mode执行trap回归，根据前文定义，将以supervisor模式执行main.c的main函数
    asm volatile("mret");

}