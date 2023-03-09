#include "risc_v.h"
#include "type.h"

void main(){

    // 仅需要init一次，所以只在harrid为0的地方进行init
    if(read_tp() == 0){
        kalloc_init()
        kvm_init();// 生成kernel pagetable
        kvm_page_start();// 开启分页内存
        trap_init();// 初始化中断
    }
}