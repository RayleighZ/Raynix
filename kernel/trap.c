#include "risc_v.h"

static int SOFTWARE_INTER = 0;
static int TIMER_INTER = 1;
static int EXTERNAL_INTER = 2;

void trap_init(){
    write_stvec((uint64)ktraph);// 指向kernel trap
}

int get_inter_type(uint64 scause){
    if((scause & 0x8000000000000000L) && (scause & 0xff) == 9){
        return EXTERNAL_INTER;
    } else if (scause == 0x8000000000000001L){
        return TIMER_INTER;
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

// 处理用户层中断
void user_trap(){

}