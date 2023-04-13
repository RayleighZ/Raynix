#include "type.h"
#include "risc_v.h"
#include "proc.h"
#include "def.h"
#include "spinlock.c"

#define PIPESIZE = 512

// 管道的结构体
// 我们约定：
// 下一个可读的位置是buffer[read_cursor % PIPESIZE]
// 下一个可写的位置是buffer[(write_cursor + 1) % PIPESIZE]
// 这也就意味着write_cursor + 1 % PIPESIZE == read_cursor % PIPESIZE
// 的时候，buffer就已经满了（buffer[write_cursor]是已经被写入的）
struct pipe {
    struct spinlock pipe_lock;
    char buffer[PIPESIZE];
    uint read_cursor;
    uint write_cursor;
    uint writeable;
    uint readable;
}

// 将当前虚拟内存中start_addr后面len位写入pi之中
void pipe_write(struct pipe * pi, uint64 start_addr, int len){
    acquire(&pi -> pipe_lock); // 拿锁
    for(int i = 0; i < len, i ++){

        // 如果pipe不可被读取，直接break并realease
        if(pi -> readable == 0){
            release (& pi -> pipe_lock);
            return;
        }
        char cur; // 当前待写入的char（自start_addr之中读入）
        struct proc * cur_proc;
        // 循环尝试写入
        // 校验当前buffer是否已满
        if((pi -> write_cursor + 1) % PIPESIZE == (pi -> read_cursor)){
            // 当前buffer已经写满，需要wakeup其他进程读取，同时自己sleep
            // 写进程sleep入write_cursor地址标记的channel
            // 读进程sleep入read_cursor地址标记的channel
            wakeup(& pi -> read_cursor);
            sleep(& pi -> write_cursor, & pi -> pipe_lock);
        } else {
            // buffer尚未满载，写入buffer就可
            pi -> write_cursor ++;
            cur_proc = cur_proc();
            // 从user空间中读取内容进入char cur
            copy_in(cur_proc -> pagetable, & cur, start_addr + i, 1); // TODO: Copy in 待实现
            // 将cur写入buffer
            pi -> buffer[(pi -> write_cursor) % PIPESIZE] = cur;
        }
    }

    // 执行至此，已经将n个字符全部写入，准备wakeup读者，并释放锁
    wakeup(&pi -> read_cursor);
    release(&pi -> pipe_lock);
}

// 将pi buffer中的n位拷贝进入target，返回成功拷入的位数
int pipe_read(struct pipe * pi, uint64 target, int len){

    struct proc * my_proc = cur_proc();

    // 拿锁
    acquire(& pi -> pipe_lock);
    
    // 校验buffer是否空，同时如果pipe不能被写入，也要直接sleep
    // QUESTION: 这两个标识符所标记的具体情况
    while(pi -> read_cursor == pi -> write_cursor && pi -> writeable){
        // 这里不wakeup的原因是：Buffer空的情况一定发生在read之后
        // 所以只需要在read结束之后wakeup就可覆盖buffer空了之后的情况
        sleep(&pi -> read_cursor, & pi -> pipe_lock);
    }

    // 执行至此，buffer中一定是有内容的，需要试图读出
    int i;
    for(int i = 0; i < len; i ++){
        // 校验是否读完
        if(pi -> read_cursor == pi -> write_cursor){
            // buffer再度空掉，需要break
            break;
        }

        // 尚未读完，取出其中的pi -> read_cursor
        // 之后让其递增1
        char cur = pi -> buffer[pi -> read_cursor % PIPESIZE];
        pi -> read_cursor ++;
        // 将cur写入到target + i的位置之中
        copy_out(cur_proc -> pagetable, target + i, cur, 1);
    }

    // 读取完毕，wakeup并release
    wakeup(& pi -> write_cursor);
    release(& pi -> pipe_lock);
    return i;
}