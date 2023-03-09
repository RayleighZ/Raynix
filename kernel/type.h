typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

#define SSTATUS_SIE (1L << 1) // supervisor中断是否开启
#define SSTATUS_UIE (1L) // user中断是否开启