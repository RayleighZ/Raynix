// UART 16550a的驱动

#include "types.h"
#include "memlayout.h"

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
#define LCR_BAUD_LATCH (1<<7) 
#define LSR 5                 
#define LSR_RX_READY (1<<0)   
#define LSR_TX_IDLE (1<<5)

// 写入内容到指定位置的memory-mapped register
void write_uart_register(int offset,unsigned char x){
    * find_uart_register(offset) = x;
}

// uart驱动初始化程序
void uart_init(){
    // 禁用中断
    write_uart_register(IER, 0x00);

    // 开启LCR bit 7，
}