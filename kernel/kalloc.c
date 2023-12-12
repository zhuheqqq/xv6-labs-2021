// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"


#define LOCK_NAME_LEN 16
char kmem_lock_name[NCPU][LOCK_NAME_LEN];

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];//面向NCPU个cpu核心的空闲列表

void
kinit()
{
  for(int i=0;i<NCPU;i++){
    char *name=kmem_lock_name[i];
    snprintf(name, LOCK_NAME_LEN - 1, "kmem_cpu_%d", i);
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  push_off();//禁用中断
  int cpu=cpuid();
  pop_off();
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_cpu(p,cpu);
}

void kfree_cpu(void *pa, int cpuid)
{
  struct run *r;

  if(((uint64)pa) % PGSIZE!= 0||(char*)pa < end||(uint64)pa > PHYSTOP)
  {
    panic("kfree");
  }

  memset(pa,1,PGSIZE);

  r=(struct run*)pa;

  acquire(&kmem[cpuid].lock);
  r->next=kmem[cpuid].freelist;
  kmem[cpuid].freelist=r;
  release(&kmem[cpuid].lock);

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  // struct run *r;

  // if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
  //   panic("kfree");

  // // Fill with junk to catch dangling refs.
  // memset(pa, 1, PGSIZE);

  // r = (struct run*)pa;

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);

  push_off();//禁用中断
  int cpu=cpuid();
  pop_off();
  kfree_cpu(pa,cpu);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cpu=cpuid();//获取当前cpu编号
  pop_off();

  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;//从当前cpu自由列表中获取一个可用的内存块
  if(r)//如果当前空闲列表不为空
  {
    kmem[cpu].freelist = r->next;//更新自由列表
    release(&kmem[cpu].lock);
  }else{//如果为空
    release(&kmem[cpu].lock);//释放内存池锁
    for(int i=0;i<NCPU;i++){//在其他cpu自由列表中查找可用的内存块
      if(i==cpu)//跳过当前cpu
      {
        continue;
      }
      acquire(&kmem[i].lock);
      r=kmem[i].freelist;

      if(r)//如果找到更新列表并释放内存池锁
      {
        kmem[i].freelist = r->next;
        release(&kmem[i].lock);
        break;
      }
      release(&kmem[i].lock);
    }
  }
    
  // 如果成功获取到内存块，使用 5 填充内存块内容
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
