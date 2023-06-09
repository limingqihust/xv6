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
#define NBUCKET 13
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
  struct buf bucket[NBUCKET];
  struct spinlock bucket_lock[NBUCKET];
} bcache;

void
binit(void)
{
  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(int i=0;i<NBUCKET;i++)
  {
    initlock(&bcache.bucket_lock[i],"bcache_bucket");
    bcache.bucket[i].prev=&bcache.bucket[i];
    bcache.bucket[i].next=&bcache.bucket[i];
  }
  for(int i=0;i<NBUF;i++)
  {
    int bucket_idx=i % NBUCKET;
    bcache.buf[i].next=bcache.bucket[bucket_idx].next;
    bcache.buf[i].prev=&bcache.bucket[bucket_idx];
    initsleeplock(&bcache.buf[i].lock,"buffer");
    bcache.bucket[bucket_idx].next->prev=&bcache.buf[i];
    bcache.bucket[bucket_idx].next=&bcache.buf[i];
  }
}
 
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  // printf("bget here\n");
  struct buf *b;
  // Is the block already cached?
  int bucket_idx=blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket_idx]);
  for(b = bcache.bucket[bucket_idx].next;b!=&bcache.bucket[bucket_idx];b=b->next)
  {
    if(b->dev==dev && b->blockno==blockno)
    {
      b->refcnt++;
      release(&bcache.bucket_lock[bucket_idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 在当前bucket中寻找一个entry
  for(b=bcache.bucket[bucket_idx].prev;b!=&bcache.bucket[bucket_idx];b=b->prev)
  {
    if(b->refcnt==0)
    {
      b->dev=dev;
      b->blockno=blockno;
      b->valid=0;
      b->refcnt=1;
      release(&bcache.bucket_lock[bucket_idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 当前bucket没有空闲位置 从其他bucket中寻找一个空闲位置
  release(&bcache.bucket_lock[bucket_idx]);
  int steal_bucket_idx=0;
  for(;steal_bucket_idx<NBUCKET;steal_bucket_idx++)
  {
    if(steal_bucket_idx==bucket_idx)
      continue;
    acquire(&bcache.bucket_lock[steal_bucket_idx]);
    for(b = bcache.bucket[steal_bucket_idx].prev;b!=&bcache.bucket[steal_bucket_idx];b=b->prev)
    {
      if(b->refcnt==0)
      {
        goto FIND;
      }
    }
    // 在当前bucket没有找到空闲的entry
    release(&bcache.bucket_lock[steal_bucket_idx]);
  }
  // 没有找到空闲的
  panic("bget: no buffers");

FIND:
  b->next->prev=b->prev;
  b->prev->next=b->next;
  release(&bcache.bucket_lock[steal_bucket_idx]);
  acquire(&bcache.bucket_lock[bucket_idx]);
  b->next=bcache.bucket[bucket_idx].next;
  b->prev=&bcache.bucket[bucket_idx];
  bcache.bucket[bucket_idx].next->prev=b;
  bcache.bucket[bucket_idx].next=b;
  b->dev=dev;
  b->blockno=blockno;
  b->valid=0;
  b->refcnt=1;
  release(&bcache.bucket_lock[bucket_idx]);
  acquiresleep(&b->lock);
  return b;
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
  // printf("brelse here\n");
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket_idx=b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket_idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bucket[bucket_idx].next;
    b->prev = &bcache.bucket[bucket_idx];
    bcache.bucket[bucket_idx].next->prev = b;
    bcache.bucket[bucket_idx].next = b;
  }
  release(&bcache.bucket_lock[bucket_idx]);
}

void
bpin(struct buf *b) {
  int bucket_idx=b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket_idx]);
  b->refcnt++;
  release(&bcache.bucket_lock[bucket_idx]);
}

void
bunpin(struct buf *b) {
  int bucket_idx=b->blockno % NBUCKET;
  acquire(&bcache.bucket_lock[bucket_idx]);
  b->refcnt--;
  release(&bcache.bucket_lock[bucket_idx]);
}


