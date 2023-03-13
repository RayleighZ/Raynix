struct spinlock{
    int available; // 当前lock是否可用
    // 每一个acqure都应该尝试向available写入0
    // 每一次release都应该尝试向available写入1
}