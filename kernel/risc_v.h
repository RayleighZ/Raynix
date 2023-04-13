#ifndef __ASSEMBLER__
uint64 read_mstatus();

// 封装在risc_v.c的汇编函数
void write_mstatus(uint64 status);
void write_satp(uint satp);
uint64 read_satp();
uint64 read_mhartid();
void write_tp(uint64 id);
uint64 read_tp();
uint64 read_stvec();
void write_stvec();
uint64 read_scause();
void write_mepc(uint64 mepc);
void write_mscratch(uint64 x);
uint64 read_mstatus();
void write_mstatus(uint64 x);
uint64 read_mie();
void write_mie(uint64 x);
void write_sepc(uint64 x);
uint64 read_sepc();
void write_mtvec(uint64 handler);

// 单页物理内存大小
#define PGSIZE 4096

// PTE的权限
#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

// machine mode的中断使能
// 写入mie寄存器
#define MIE_MEIE (1L << 11)
#define MIE_MTIE (1L << 7) 
#define MIE_MSIE (1L << 3) 

// supervisor mode的中断使能
#define SSTATUS_SPP (1L << 8)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_UPIE (1L << 4)
#define SSTATUS_SIE (1L << 1)
#define SSTATUS_UIE (1L << 0)

// 内存对齐
#define PGROUNDUP(sz) (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

typedef uint64 pte_t;
typedef uint64 * pagetable_t;//包含512个PTE的pagetable

#endif // __ASSEMBLER__

// 可能的最大虚拟内存地址
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))