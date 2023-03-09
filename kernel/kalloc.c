#include "type.h"

struct free_page_link{
    struct free_page_link * next;
}

struct {
    struct spinlock lock;
    struct free_page_link * page_link;
} page_list_keeper;

// extern char mem_start [];//内存位置的开始

free_page_link * fp;

// 将pa 4kb对齐之后的一页free
void free_page(void * pa){
    uint64 pa_start = PGROUNDUP(pa);
    set_memory(pa_start, 1, 4096);
    // 将pa头插到节点
    free_page_link * fpl = (struct free_page_link *) pa;
    fpl->next = page_list_keeper.page_link;
    page_list_keeper.page_link = fpl;
}

void * kalloc(){
    struct free_page_link * fp;

    acquire(&page_list_keeper.lock);
    // 如果是非空，则取出头
    fp = page_list_keeper.page_link;
    if(fp){
        page_list_keeper.free_page_link = fp->next;
    }
    release(&page_list_keeper.lock);

    set_memory((char *)fp, 5, 4096);
    return (void*)fp;
}

void kalloc_init(){
    // 清理出从KERNBASE到PHYSTOP的内存
    for (uint64 pa = KERNBASE; pa <= PHYSTOP; pa += 4096){
        free_page((void *) pa);
    }
    return 0;
}