// kalloc.c
void * kalloc();
void kalloc_init();

// vm.c
void kvm_init();
void kvm_page_start();

// trap.c
void trap_init();

// proc.c
void init_proc_kstack(pagetable_t pgt);
