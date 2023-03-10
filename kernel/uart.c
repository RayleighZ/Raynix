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
#define LSR_RX_READY (1<<0)   
#define LSR_TX_IDLE (1<<5)

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

int uart_get_char

// uart中断处理
void uart_inter(){
    // 不断循环从RHR读取字符，直到\n或者Ctrl+C
    int flag = 1;
    while(flag){
        // 先判断是否RHR可读
        if(read_uart_register(LSR) & 0x01){
            // 证明可以读取
            int res =  read_uart_register(RHR);
            // 交给consoleintr处理，值得注意的是，这里并没有触发真正的中断
            // 仅仅是调用了中断函数处理
            // 来到此处，驱动的low side作用已经完成
            console_intr(res);
        } else {
            // 已经完整读完了一次，可以终止循环
            flag == 0;
        }
    }
}