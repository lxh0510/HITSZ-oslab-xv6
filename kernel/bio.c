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


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13               // 设置哈希桶的个数

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashbucket[NBUCKETS]; // 每个哈希队列一个linked list及一个lock，其linked list是一个双向链表
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;i++)                                  // 对每个哈希桶进行初始化
  {
    initlock(&bcache.lock[i], "bcache");                       // 初始化哈希桶的锁
  // Create linked list of buffers 
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];         // 初始化哈希桶的双向链表
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){             // 将每个缓存块按照哈希函数分配到其对应哈希桶中
    int i = b->blockno%NBUCKETS;
    b->next = bcache.hashbucket[i].next;
    b->prev = &bcache.hashbucket[i];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[i].next->prev = b;
    bcache.hashbucket[i].next = b;
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketno = blockno % NBUCKETS;                       // 获取该缓存块对应哈希桶号
  acquire(&bcache.lock[bucketno]);                         // 给该哈希桶加锁

  // Is the block already cached?
  for(b = bcache.hashbucket[bucketno].next; b != &bcache.hashbucket[bucketno]; b = b->next){     // 遍历哈希桶中所有缓存块
    if(b->dev == dev && b->blockno == blockno){                                                  // 若命中了cache
      b->refcnt++;                                        // 该块被引用次数加一
      release(&bcache.lock[bucketno]);                    // 释放哈希桶的锁
      acquiresleep(&b->lock);                             // 给该缓存块加睡眠锁
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 若未命中
  // 首先在原来哈希桶中找空闲内存

  for(b = bcache.hashbucket[bucketno].prev; b != &bcache.hashbucket[bucketno]; b = b->prev){     
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[bucketno]);                  // 释放哈希桶的锁
      acquiresleep(&b->lock);                           // 给该缓存块加睡眠锁
      return b;
    }
  }

  // 若原来哈希桶也没有，则从其他哈希桶中“偷取”空闲内存

  for(int i=0;i<NBUCKETS;i++)                           // 遍历其他哈希桶
  {
    if(i!=bucketno){
      acquire(&bcache.lock[i]);                         // 给该哈希桶加锁
      for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){       // 遍历该哈希桶寻找内存块
        if(b->refcnt == 0) {                            // 若找到空闲内存
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;

          b->next->prev=b->prev;                         //从原哈希桶中pop出来
          b->prev->next=b->next;                        


          b->next = bcache.hashbucket[bucketno].next;    //push到以bucketno为key的哈希桶中
          b->prev = &bcache.hashbucket[bucketno];
          bcache.hashbucket[bucketno].next->prev = b;
          bcache.hashbucket[bucketno].next = b;
          release(&bcache.lock[i]);                      // 释放该哈希桶加锁
          release(&bcache.lock[bucketno]);               // 释放原哈希桶的锁
          acquiresleep(&b->lock);                        // 给该缓存块加睡眠锁
          return b;
        }
      }
      release(&bcache.lock[i]);
    }
  }
  release(&bcache.lock[bucketno]);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
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

  releasesleep(&b->lock);                            // 释放该缓存块的睡眠锁
  int bucketno = b->blockno%NBUCKETS;                // 获取该缓存块的哈希桶号
  acquire(&bcache.lock[bucketno]);                   // 给该哈希桶加锁
  b->refcnt--;                                       // 该块被引用次数减一
  if (b->refcnt == 0) {                              // 若该缓存块成为了空闲内存
    // no one is waiting for it.
    b->next->prev = b->prev;                           // 将其移入空闲内存列表的头部
    b->prev->next = b->next;
    b->next = bcache.hashbucket[bucketno].next;
    b->prev = &bcache.hashbucket[bucketno];
    bcache.hashbucket[bucketno].next->prev = b;
    bcache.hashbucket[bucketno].next = b;
  }
  
  release(&bcache.lock[bucketno]);                 // 释放该哈希桶的锁
}

void
bpin(struct buf *b) {
  int bucketno = b->blockno%NBUCKETS;               // 改为对缓存块所在哈希桶进行加锁解锁操作
  acquire(&bcache.lock[bucketno]);
  b->refcnt++;
  release(&bcache.lock[bucketno]);
}

void
bunpin(struct buf *b) {
  int bucketno = b->blockno%NBUCKETS;               // 改为对缓存块所在哈希桶进行加锁解锁操作
  acquire(&bcache.lock[bucketno]);
  b->refcnt--;
  release(&bcache.lock[bucketno]);
}


