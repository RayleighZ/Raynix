// 执行虚拟内存加载相关
// TODO: kalloc函数尚未完成
#include "type.h"
#include "risc_v.h"
#include "memlayout.h"
#include "def.h"
#include "tools.h"

// kernel的页表
pagetable_t kernel_pagetable;

// 在kernel.ld中定义的内核结束位置
extern char etext[];

// trampoline.S里定义的trampoline位置
extern char trampoline[];

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
            pagetable = (pte_t*)kalloc();
            set_memory(pagetable, 0, PGSIZE);// 清零的是页表指向的内容，而非页表本身
            *cur_pte = (((uint64) pagetable >> 12) << 10) | PTE_V;
        }
    }
    // 这里跳出来的时候是最后一级的pagetable，跳出的目的是不对其进行kalloc操作
    return &pagetable[(va >> 12) & mask_index];
}

// va转译pa
uint64 find_pa(pagetable_t pagetable, uint64 va){
    // 找到va对映的pte
    pte_t * pte = walk(pagetable, va);
    // 利用pte和offset查询pa
    uint64 offset_mask = 0xFFF;
    uint64 pa = ((((uint64) * pte) >> 10) << 12 | (va & offset_mask));
    return pa;
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
        *pte_cur = (((uint64) pa >> 12) << 10) | permission | PTE_V;
        va_start += 4096;
        pa += 4096;
    }
}

void fatal_pa_map(){
    // uart
    map(UART0, UART0, kernel_pagetable, PGSIZE, PTE_R | PTE_W);
  
    //virtio mmio disk
    map(VIRTIO0, VIRTIO0, kernel_pagetable, PGSIZE, PTE_R | PTE_W);
  
    // PLIC
    map(PLIC, PLIC, kernel_pagetable, 0x400000, PTE_R | PTE_W);
  
    // kernel text
    map(KERNBASE, KERNBASE, kernel_pagetable, (uint64)etext-KERNBASE, PTE_R | PTE_X);
  
    // map kernel data
    map((uint64)etext, (uint64)etext, kernel_pagetable, PHYSTOP-(uint64)etext, PTE_R | PTE_W);
  
    // trampoline, user kernel切换中断时使用
    map(TRAMPOLINE, (uint64)trampoline, kernel_pagetable, PGSIZE, PTE_R | PTE_X);
}

// 将pagetable中addr虚拟内存之后len位拷贝进入target之中
// 用于将user的pagetable的内容cv进入kernel中处理
void copy_in(pagetable_t pagetable, char * target, uint64 addr, uint64 len){
    // 先拿到物理内存地址
    // 这里转换为pa的原因是：在kernel中内存几乎是直接映射
    uint64 pa_star = find_pa(pagetable, addr);
    memmove(target, (void *)(pa_star), len);
}

// 同上，将source写入到target之中
void copy_out(pagetable_t pagetable, uint64 target, char * source, uint64 len){
    uint64 pa_star = find_pa(pagetable, target);
    memmove(source, (void *)pa_star, len);
}

void kvm_init(){
    // 分配pagetable地址
    // 值得注意的是这本身就是一个数组
    kernel_pagetable = (pagetable_t) kalloc();
    
    // 对pagetable进行置零
    set_memory(kernel_pagetable, 0, PGSIZE);

    // 对必要的物理内存地址进行映射
    fatal_pa_map();

    // 为每一个进程分配kernel stack
    init_proc_kstack(kernel_pagetable);
}

// 不整合入kvm_init的原因是
// 只有hartid 0需要生成pagetable
// 其他hart只需要写入pagetable并开启分页就可以了
void kvm_page_start(){
    // 开启分页内存
    // sfence_vma();
    // kernel_pagetable本身就是uint64指针
    // 这意味着只需要将地址转换成satp寄存器想要的样子就可以了
    // satp寄存器的60~63位标志着寻址模式，这里我们采用的是sv39
    // 所以需要将mode设置为0x8
    uint64 sv39_mode = (0x8l << 60);

    // 剩下的部分，我们只要扬弃这个pa的12位offset，仅留下PPN写入就可
    write_satp((((uint64)kernel_pagetable) >> 12) | sv39_mode);
    // sfence_vma();
}

// 生成一页空的页表
pagetable_t empty_vm_create(){
    // 申请一页页表
    pagetable_t pagetable;
    pagetable = (pagetable_t) kalloc();

    // 请空页表
    set_memory(pagetable, 0, PGSIZE);
    return pagetable;
}