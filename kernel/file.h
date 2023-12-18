struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
  uint dev;           // Device number设备、号
  uint inum;          // Inode number   inode号
  int ref;            // Reference count  引用计数
  struct sleeplock lock; // protects everything below here睡眠锁
  int valid;          // inode has been read from disk?标志位表示inode是已经从磁盘读取到内存

  short type;         // copy of disk inode  文件类型
  short major;   //主设备号
  short minor;    //次设备号
  short nlink;    //链接数
  uint size;      //
  uint addrs[NDIRECT+2];//存储文件数据块的数组
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
