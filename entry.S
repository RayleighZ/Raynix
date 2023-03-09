.section .text
.global _entry
_entry:
        # 分配后续运行start.c所需要的栈
        # 单个进程分配4096 byte的栈
        la sp, stack0
        li a0, 1024*4
        csrr a1, mhartid
        # risc-v的栈向下增长
        # 所以第一块栈的起始地址应该是stack0+4096*核心数
        # stack0位于start.c之中
        addi a1, a1, 1
        mul a0, a0, a1
        # 因为这是第一个机械进程
        # 所以mhartid应当为0
        # 故这里是将sp指向stack0+4096
        add sp, sp, a0
        call start
spin:
        j spin