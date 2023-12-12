struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;//设备对应的标识符
  uint blockno;//缓冲区对应的块号
  struct sleeplock lock;//一个睡眠锁用于在对缓冲区进行读写操作时进行同步
  uint refcnt;//缓冲区的引用计数
  //struct buf *prev; // LRU cache list
  struct buf *next;//指向下一个缓冲区的指针
  uchar data[BSIZE];//用于存储实际的数据内容
  uint timestamp;//时间戳标志
};

