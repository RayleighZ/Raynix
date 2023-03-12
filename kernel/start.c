#include "type.h"
#include "risc_v.h"

void main();

void start(){
    // 当前hart处于machine模式（通过trap来到这里）
    // 应当在完成初始配置后回到supervisor模式，并进行后续内核启动
    
    // 将mstatus寄存器中trap来源的权限状态设置为supervisor
    // 以便在machine模式从trap中返回时可以正确处于supervisor mode
    unsigned long mstatus = read_mstatus();
    mstatus &= !(3L << 11);//将mstatus的11 12位MPP清零
    mstatus += 1L << 11;//将上一个状态设置为supervisor

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