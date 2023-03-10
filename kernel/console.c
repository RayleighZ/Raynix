#include "types.h"

struct {
    struct spinlock lock;
    char buf[128];// 输入输出缓冲数组
} console;

// 处理来自uart的输入中断
void consloe_input_handler(int c){
    acquire(&console.lock);
    // 除了一些必要的操作字符之外，其他的将写入console.buf之中
}