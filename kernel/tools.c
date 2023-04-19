#include "type.h"

// 工具方法合集
void * set_memory(void *start, int value, uint length){
    // 暴力转型
    char *tar_start = (char *) start;
    for(int i = 0; i < length; i++){
        tar_start[i] = value;
    }
    return start;
}

void*
memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  if(n == 0)
    return dst;
  
  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}