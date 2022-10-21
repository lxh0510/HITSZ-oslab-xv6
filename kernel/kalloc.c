// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];     // 每个cpu拥有一个结构体kmems，其中包含各自的锁lock和空闲内存链表freelist

void
kinit()
{
  for(int i=0;i<NCPU;i++){
    initlock(&kmems[i].lock, "kmem");      // 将每个cpu的锁进行初始化
  }
  freerange(end, (void*)PHYSTOP);
}

// freerange为所有运行freerange的CPU分配空闲的内存
void
freerange(void *pa_start, void *pa_end)     
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  // 关闭中断并获取cpuid
  push_off();
  int cid=cpuid();

  r = (struct run*)pa;

  acquire(&kmems[cid].lock);    
  r->next = kmems[cid].freelist;   // 使用头插法将空闲内存插入空闲内存链表中
  kmems[cid].freelist = r;
  release(&kmems[cid].lock);

  pop_off();             //打开中断
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

void *
kalloc(void)
{
  struct run *r;

  // 关闭中断并获取cpuid
  push_off();
  int cid=cpuid();

  acquire(&kmems[cid].lock);   // 加锁 
  r = kmems[cid].freelist;  // 获取cpuid为cid的cpu的空闲内存链表

  if(r)
    kmems[cid].freelist = r->next;  // 如果空闲链表有空闲内存，则将链表中首个空闲内存弹出
  else
  {
    int find=0;                         // find用于标记是否在其他cpu的空闲内存链表找到了空闲内存
    for(int i=0;i<NCPU;i++)             // 遍历每一个cpu
    {
      if(i==cid)    continue;
      acquire(&kmems[i].lock);          // 对该cpu上锁
      r = kmems[i].freelist;            // 获取该cpu的空闲内存链表
      if(r)                             // 如果空闲链表有空闲内存，则将链表中首个空闲内存弹出
      {
        kmems[i].freelist = r->next;
        find=1;                         // find标记为1，表示找到了空闲内存 
        release(&kmems[i].lock);        // 在return前需要解锁
        break;
      }
      release(&kmems[i].lock);         // 未找到空闲内存也需要解锁
    }
    if(find==0){                      // 若遍历所有cpu均未找到
      release(&kmems[cid].lock);      // 将cid对应cpu解锁
      pop_off();                      // 打开中断
      return 0;
    } 
  }

  release(&kmems[cid].lock);         // 将cid对应cpu解锁
  pop_off();                         // 打开中断

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

