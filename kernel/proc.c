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