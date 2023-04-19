// 因为自旋锁是busy的
// 所以持有spinlock者并不能持有它很长时间
// 尤其是不能sleep
// 而对于需要长期持有锁的行为，需要引入sleeplock
// 其坏处是，因为线程会被挂起
// 将会消耗比spinlock更长的时间

#include "type.h"
#include "def.h"

void acquire_sleep(struct sleeplock * sllk){
    // 尝试获取sllk的splk
    acquire(&sllk -> splk);
    while(!sllk -> holding){
        // 这里与自旋动作类似，不断的sleep直到无人holding
        // acquire spinlock的目的是：
        // 对sllk->holding的校验是需要保证critical的
        sleep(sllk, &sllk -> splk);
        // 值得一提，这里也是一种所谓交叉锁的思想
        // 因为当sleep结束的时候，会重新拿回锁

        // 从这里也可以看出，sleeplock不是那么的“急”
        // 所以比起使用spinlock，使用sleeplock将会更慢拿到lock
    }

    // 执行至此，sleeplock已经夺得了sllk->holding
    // 并且仍然持有splk，所以这里可以安全的修改sllk的holding状态
    splk -> holding = 1;
    // 可以释放splk，执行critical的任务
    release(&sllk -> splk);
}

void release_sleep(struct sleeplock * sllk){
    // 同样是先试图拿到spinlock
    acquire(& sllk -> splk);
    // 简单修改一下lock的状态
    sllk -> holding = 0;
    wakeup(sllk);
    release(& sllk -> splk);
}