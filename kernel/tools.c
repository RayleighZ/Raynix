#include "types.h"

// 工具方法合集
void* set_memory(void *start, int value, uint length){
    // 暴力转型
    char *tar_start = (char *) start;
    for(int i = 0; i < length; i++){
        tar_start[i] = value;
    }
    return start;
}