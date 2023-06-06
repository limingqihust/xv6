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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct page_lock
{
  struct spinlock lock;
  int cnt[557056+100];
}page_ref;
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_ref.lock,"page_ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}
int ref_cnt[557056+100];
// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  
  // ref_cnt[(uint64)pa/PGSIZE]=ref_cnt[(uint64)pa/PGSIZE]==0?0:(ref_cnt[(uint64)pa/PGSIZE]-1);//���ü���-1
  // if(ref_cnt[(uint64)pa/PGSIZE])    // ���ü���!=0
  //   return ;
  acquire(&page_ref.lock);
  page_ref.cnt[(uint64)pa/PGSIZE]=page_ref.cnt[(uint64)pa/PGSIZE]==0?0:(page_ref.cnt[(uint64)pa/PGSIZE]-1);//���ü���-1
  if(page_ref.cnt[(uint64)pa/PGSIZE])    // ���ü���!=0
  {
    release(&page_ref.lock);
    return ;
  }
  release(&page_ref.lock);
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  // ref_cnt[(uint64)r/PGSIZE]++;    // ���ü���+1 
  acquire(&page_ref.lock);
  page_ref.cnt[(uint64)r/PGSIZE]++;
  release(&page_ref.lock);
  return (void*)r;
}
