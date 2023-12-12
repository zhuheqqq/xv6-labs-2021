// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//修改LRU本身的双向循环链表 

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define LOCK_NAME_LEN 20
char bucket_lock_name[NBUCKET][LOCK_NAME_LEN];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct bucket hashtable[NBUCKET];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

struct bucket{//哈希桶
  struct spinlock lock;
  struct buf head;
};

int hash(uint dev,uint blockno)
{
  return (dev + blockno) % NBUCKET;
}

void
binit(void)
{
  struct buf *b;
  struct bucket *bucket;

  initlock(&bcache.lock, "bcache");//初始化锁并命名为bcache

  for(int i=0;i<NBUCKET;i++){//遍历每个哈希桶
    char *name=bucket_lock_name[i];
    //为每个哈希桶的锁生成一个独特的名称
    snprintf(name,LOCK_NAME_LEN-1,"bcache_bucket_%d",i);//格式化字符串生成一个锁的名称并存储在name变量中
    initlock(&bcache.hashtable[i].lock,name);//初始化锁并命名为bcache_bucket_i
  }
  b=bcache.buf;
  //初始化缓冲区池中的每个缓冲区
  for(int i=0;i<NBUF;i++)
  {
    initsleeplock(&bcache.buf[i].lock,"buffer");
    bucket=bcache.hashtable+(i%NBUCKET);//计算当前缓冲区属于哪个哈希桶
    //将当前缓冲区添加到该哈希桶的缓冲区链表头部代表最近使用的链表
    b->next=bucket->head.next;
    bucket->head.next=b;
    b++;
  }

  // Create linked list of buffers双向循环链表
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;

  // //遍历缓冲区数组 并初始化
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");//初始化缓冲区睡眠锁
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b,*prev_replace_buf;
  struct bucket *bucket,*prev_bucket,*cur_bucket;

  int index=hash(dev,blockno);
  bucket=bcache.hashtable+index;//计算当前缓冲区属于哪个哈希桶

  acquire(&bucket->lock);

  // Is the block already cached?
  //检查块是否在缓冲区中,首先遍历链表，查找有没有信息一致直接命中的缓存块
  for(b = bucket->head.next; b ; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;//如果在 增加引用计数
      release(&bucket->lock);//释放缓冲区的锁
      acquiresleep(&b->lock);//获取缓冲区的睡眠锁
      return b;//返回找到的缓冲块
    }
  }

  release(&bucket->lock);//释放缓冲区的锁

  acquire(&bcache.lock);//获取缓冲区的锁
  acquire(&bucket->lock);

  // Not cached.如果块不在缓冲区中
  // Recycle the least recently used (LRU) unused buffer.
  //回收最近最少使用的未使用的缓冲块
  for(b = bucket->head.next; b; b = b->prev){
    if(b->dev==dev&&b->blockno) {
      // b->dev = dev;
      // b->blockno = blockno;
      // b->valid = 0;//有效标志置0
      // b->refcnt = 1;//将当前缓冲块引用计数设为1
      b->refcnt++;
      release(&bucket->lock);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bucket->lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);// 调用 bget 函数获取指定设备和块号的缓冲块
  // 检查缓冲块的有效标志，如果为假，则从块设备中读取数据
  if(!b->valid) {
    virtio_disk_rw(b, 0);// 使用 virtio_disk_rw 函数从块设备中读取数据到缓冲块
    b->valid = 1;// 将缓冲块的有效标志设置为真，表示数据已经有效
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }

  int index=hash(b->dev,b->blockno);//获得当前缓冲区编号
  accquire(&bcache.hashtable[index].lock);
  b->refcnt--;
  if(b->refcnt==0)
  {
    b->timestamp=ticks;
  }
  
  release(&bcache.hashtable[index].lock);
}

void
bpin(struct buf *b) {
  int index=hash(b->dev,b->blockno);//获得当前缓冲区编号
  acquire(&bcache.hashtable[index].lock);
  b->refcnt++;
  release(&bcache.hashtable[index].lock);
}

void
bunpin(struct buf *b) {
  int index=hash(b->dev,b->blockno);//获得当前缓冲区编号
  acquire(&bcache.hashtable[index].lock);
  b->refcnt--;
  release(&bcache.hashtable[index].lock);
}


