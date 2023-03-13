// 读写mstatus寄存器
inline uint64 read_mstatus(){
    uint64 status;
    asm volatile("csrr %0, mstatus" : "=r" (status));
    return status;
}

inline void write_mstatus(uint64 status){
    asm volatile("csrw mstatus, %0" : : "r" (status));
}

// 写mepc寄存器（控制异常中断函数指针位置）
inline void write_mepc(uint64 mepc){
    asm volatile("csrw mepc, %0" : : "r" (mepc));
}

// 写satp寄存器（控制是否分页寻址）
inline void write_satp(uint64 satp){
    asm volatile("csrw satp, %0" : : "r" (satp));
}

// 写入TrapHandler的位置
inline void write_stvec(uint64 x){
    asm volatile("csrw stvec, %0" : : "r" (x));
}

// 读取TrapHandler的位置
inline uint64 read_stvec(){
    uint64 res;
    asm volatile("csrr %0, stvec" : : "=r" (res));
    return res;
}

// 获取当前mhartid
// 值得注意的是，当进入supervisor mode之后
// 系统就将不具备访问此方法的能力
// 故需要额外根据线程指针来缓存一份所谓的cpuid
inline uint64 read_mhartid(){
    uint64 id;
    asm volatile("csrr %0, mhartid" : "=r" (id) );
    return id;
}

// 读写tp，这里的目的是用其缓存cpuid
// 写入的任务将在machine_mode执行
// 以便在后期supervisor_mode下可以轻松获取到当前的cpuid
inline void write_tp(uint64 id){
    asm volatile("mv tp, %0" : : "r" (id));
}

inline uint64 read_tp(){
    uint64 id;
    asm volatile("mv tp, %0" : "=r" (id));
    return id;
}

inline uint64 read_sstatus(){
    uint64 x;
    asm volatile("csrr sstatus, %0" : "=r" (x));
    return x;
}

// 读取中断诱因
inline uint64 read_scause(){
    uint64 cause;
    asm volatile("csrr %0, scause" : "=r" (cause));
    return cause;
}

// 校验是否开启了中断
inline int check_open_interrupts(){
    return (read_sstatus & SSTATUS_SIE) != 0;
}

// 写入machine mode的scratch
inline void write_mscratch(uint64 x){
    asm volatile("csrw mscartch, %0" : : "r" (x));
}

// 写入时钟中断handler
inline void write_mtvec(uint64 handler){
    asm volatile("csrw mtvec, %0" : : "r" (handler));
}

// 读写Machine Status Register
// 控制Machine mode的整体中断使能
inline uint64 read_mstatus(){
    uint64 result;
    asm volatile("csrr %0, mstatus" : "=r" (result));
    return result;
}

inline void write_mstatus(uint64 x){
    asm volatile("csrw mstatus, %0" : : "r" (x));
}

// 读写mie寄存器
// 控制具体类型的中断使能
inline uint64 read_mie(){
    uint64 result;
    asm volatile("csrr %0, mie" : "=r" (result));
    return result;
}

inline void write_mie(uint64 x){
    asm volatile("csrw mie, %0" : : "r" (x));
}

// 读写sstatus寄存器
// 控制supervisor mode的整体中断使能
inline uint64 read_sstatus(){
    uint64 result;
    asm volatile("csrr %0, sstatus" : "=r" (result));
    return result;
}

inline void write_sstatus(uint64 x){
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

// 刷新TLB
static inline void
sfence_vma()
{
    asm volatile("sfence.vma zero, zero");
}