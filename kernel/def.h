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

// spinlock.c
void acquire(struct spinlock * lock);
void release(struct spinlock * lock);
void reset_lock(struct spinlock * lock);
void spinlock_init();
void pop_inter_off();
void push_inter_off();
