#include "type.h"
#include "risc_v.h"
#include "def.h"

void main(){
    // 仅需要init一次，所以只在harrid为0的地方进行init
    if(read_tp() == 0){
        spinlock_init() // 初始化spinlock的两个标识符
        kalloc_init(); // 初始化可分配页表
        kvm_init(); // 生成kernel pagetable
        kvm_page_start();// 开启分页内存
        trap_init(); // 初始化中断
    }
}