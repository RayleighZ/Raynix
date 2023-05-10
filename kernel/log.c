#include "type.h"
#include "memlayout.h"
#include "risc_v.h"
#include "def.h"
#include "proc.h"

// loghead, 存储着当前log所相关的block以及block的数量
struct loghead {
    int block_size;
    int block[LOGSIZE];
};

// 具体的一条log
struct log{
    struct spinlock splk; // 保证顺序访问的lock
    struct loghead head; // 存储其他待写入block的位置的head
    int is_committing; // 文件系统是否正在commit，如果正在commit，则需要等待
    int on_excuting; // 当前文件系统中有多少sys call正在执行
}

struct log log;

// 在执行FS提供的系统调用前调用
// 用以保证：
// 1、当前并没有正在commit的操作
// 2、当前log space有充足的空间容纳下这次system call
void begain_op(){
    // 先试图拿到log的锁（全局唯一，在initlog之中被加载）
    acquire(&log.splk);
    // 校验是否满足函数开头注释中的两个条件
    // 值得注意的是，这里的已经拿到了log的spinlock
    // 也就意味着，对log相关参数的校验不用担心同步性问题
    int flag = 1;
    while(flag){
        if(log.is_committing){
            // log正在进行commit
            // 直接进行sleep（等待commit结束之后被唤醒）
            sleep(&log, &log.splk);
        } else if(log.head.block_size + (log.on_excuting + 1) * MAXOPBLOCKS){
            // 如果剩余的可用空间不足，同样需要sleep
            // 以log本身为sleep的channel，在sleep之前释放log包含的splk
            sleep(&log, &log.splk);
        } else {
            // 一切正常，可以进行相关文件操作
            log.on_excuting += 1;
            release(&log.splk);
            break;
        }
    }
}