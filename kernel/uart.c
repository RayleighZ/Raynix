// UART 16550a的驱动

#include "types.h"
#include "memlayout.h"
#include "spinlock.h"
#include "def.h"

// 从UART0位置开始找后面memory-mapped的uart寄存器
unsigned char * find_uart_register(int offset){
    return (unsigned char *) UART0 + reg;
}

// UART 16550a的寄存器位置定义
// 寄存器定义参考http://byterunner.com/16550.html
#define RHR 0                 
#define THR 0                 
#define IER 1                 
#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR 2                 
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) 
#define ISR 2                 
#define LCR 3                 
#define LCR_EIGHT_BITS (3<<0)
#define LSR 5                 

// 写buffer（transmit buffer）
// 此buffer做循环队列使用
// 自w_tbuffer_cursor % 32写入，自r_tbuffer_cusor % 32读出
char t_buffer[32];
// 事先约定，这里的buffer记录的均是上一个写入or读出的位置
// 这意味着如果要进行写入，使用的应该是cursor+1
// 如果要进行读出，使用的应该是cursor，并随后+1

// 写cursor，标志着当前buffer的写入末尾位置
uint64 w_tbuffer_cursor;
// 读cursor, 标志着当前buffer的读取位置
uint64 r_tbuffer_cursor;

// 写入内容到指定位置的memory-mapped register
void write_uart_register(int offset,unsigned char x){
    * find_uart_register(offset) = x;
}

// 读取寄存器内容
unsigned char read_uart_register(int offset){
    return * find_uart_register(offset)
}

// uart驱动初始化程序
void uart_init(){
    // 禁用中断
    write_uart_register(IER, 0x00);

    // 开启LCR bit 7，之后手动设置波特率
    write_uart_register(LCR, (1<<7));
    // 设置波特率为38.4k
    write_uart_register(0, 0x03);
    write_uart_register(1, 0x00);

    // 完成波特率设置，并设置为8bit字长
    write_uart_register(LCR, LCR_EIGHT_BITS);

    // 重置FIFO队列
    write_uart_register(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

    // 开启收发中断
    write_uart_register(IER, IER_TX_ENABLE | IER_RX_ENABLE);

    // 初始化锁
    initlock(&uart_tx_lock, "uart");
}

// 正式执行uart设备的发送指令
void uart_send(){
    // 我们将一直尝试写入，直到THR写满无法工作
    for(;;){
        if(w_tbuffer_cursor == r_tbuffer_cursor){
            // 首尾衔接，证明没有什么可send的了
            return;
        }

        // 先读取LSR的bit 5，如果为1则证明当前发送寄存器（THR）idle可以发送
        if(read_uart_register(LSR) & (1 << 5) == 0){
            // THR full，尚未准备好发送
            // 值得注意的是，当UART成功完成一次发送的时候，它还会再触发一次中断
            // 这意味这我们需要再uart_inter里面进行对send的调用
            return;
        } else {
            // THR空闲，按照FIFO的策略读出队首的char，并写入THR
            // 读出队首char
            char on_sending = t_buffer[(r_tbuffer_cursor ++) % 32];
            // 叫醒其他在sleep的写入进程
            // TODO: Wake up
            write_uart_register(THR, on_sending);
        }
    }
}

// uart的写入函数
int uart_input(int c){
    // 拿锁
    acquire(&uart_tx_lock);

    // 首先判断循环队列是否已满
    // 循环队列满的情况自然是w等于r+32
    // 值得注意的是，这里需要循环校验
    // 当uart send发送出去之后，它会随机挑选一个处于sleep之中的uart_input
    // 但是当uart_input被唤醒的时候，仍然不能完全确定其处于可以发送的状态
    // 所以仍需进行校验
    while(w_tbuffer_cursor == r_tbuffer_cursor + 32){
        // 队列已满，需要等待
        // TODO: Sleep
    }

    // 走到这里意味着可以执行写入buffer并尝试调用发送
    t_buffer[(++w_tbuffer_cursor) % 32] = c;
    // 调用uart_send
    uart_send();
}

// uart中断处理
void uart_inter(){
    // 不断循环从RHR读取字符，直到\n或者Ctrl+C
    int flag = 1;
    while(flag){
        // 先判断是否RHR可读
        if(read_uart_register(LSR) & 0x01){
            // 证明可以读取
            int res =  read_uart_register(RHR);
            // 剩下的交给console去handle
            // 来到此处，驱动的low side作用已经完成
            consloe_input_handler(res);
        } else {
            // 已经完整读完了一次，可以终止循环
            flag == 0;
        }
    }
}