// 执行虚拟内存加载相关
// TODO: kalloc函数尚未完成
#include "risc_v.h"
#include "type.h"
#include "memlayout.h"
#include "def.h"

//kernel的页表
pagetable_t kernel_pagetable;

//根据va和pagtetable查询PTE
pte_t * walk(pagetable_t pagetable, uint64 va){
    // 寻找va对应的PTE
    uint64 mask_index = 0x1FF;

    for(int i = 0; i < 2; i++){
        uint64 cur_list = (va >> (12 + 9 * (2 - i))) & mask_index;
        pte_t* cur_pte = &pagetable[cur_list];
        // 如果页表已经形成，就查询PTE下一级的物理内存地址
        // 如果没有形成，则分配新的Page和PTE
        if(*cur_pte & PTE_V){
            // 已经分配
            // 将pagetable指向下一层的pagetable
            pagetable = (pagetable_t) ((*cur_pte >> 10) << 12);
        } else {
            // 新建一个pagetable，并将其物理地址赋值给PTE
            // 同时由于这就是下一级，所以可以直接赋值给pagetable
            pagetable = (pde_t*)kalloc();
            set_memory(pagetable, 0, PGSIZE);// 清零的是页表指向的内容，而非页表本身
            *cur_pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    // 这里跳出来的时候是最后一级的pagetable，跳出的目的是不对其进行kalloc操作
    return &pagetable[(va >> 12) & mask_index];
}

// va: 虚拟内存地址 pa: 物理内存地址 pagetable 映射中va对应的PTE的根pagetable
// size: 对va往后的多少页虚拟内存进行映射 permission: PTE所具备的权限
// 将va映射为pa，并且将其后size page的内存地址进行顺序映射
void map(uint64 va, uint64 pa, pagetable_t pagetable, uint64 size, uint64 permission){

    // 当前pagetable是最后一级（第三级），下一步是对其分配4096 byte的物理地址
    // 首先进行4kb对齐
    uint64 va_start = PGROUNDDOWN(va);
    // uint64 va_last = PGROUNDDOWN(va + size * 4096);

    // 对这段va每一个分配4096 byte的物理内存，并赋值给PTE
    // 我们仅需要对每间隔4096的va进行分配，并且只需要把pa的头给他
    for (int i = 0; i < size; i++){
        //拿到当前va指代的PTE
        pte_t * pte_cur = walk(pagetable, va_start);
        // 将pa装载到当前pte中
        // 需要注意的是，我们进行的是将va的offset 0映射到pa的offset 0
        // 所以实际上是进行了size页的映射
        *pte_cur = (((uint64) pa >> 12) << 10) | perm | PTE_V;
        va_start += 4096;
        pa += 4096;
    }
    return 0;
}

void fatal_pa_map(){
    // uart
    map(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);
  
    //virtio mmio disk
    map(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
  
    // PLIC
    map(kpgtbl, PLIC, PLIC, 0x400000, PTE_R | PTE_W);
  
    // kernel text
    map(kpgtbl, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);
  
    // map kernel data
    map(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);
  
    // trampoline, user kernel切换中断时使用
    map(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
}

void kvm_init(){
    // 分配pagetable地址
    // 值得注意的是这本身就是一个数组
    k_pagetable = (pagetable_t) kalloc();
    
    // 对pagetable进行置零
    uint64 zero = 0;
    for(int i = 0, i < 512, i++){
        k_pagetable = zero;
    }

    // 对必要的物理内存地址进行映射
    fatal_pa_map();

    // 为每一个进程分配kernel stack
    init_proc_kstack(k_pagetable);
}

// 不整合入kvm_init的原因是
// 只有hartid 0需要生成pagetable
// 其他hart只需要写入pagetable并开启分页就可以了
void kvm_page_start(){
    // 开启分页内存
    // sfence_vma();
    write_satp(k_pagetable);
    // sfence_vma();
}