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
  // 上锁顺序： reflk, lock
  struct spinlock lock;
  struct spinlock reflk;
  struct run *freelist;
  int refcnt[NPHYPG];
} kmem;

// caller must held kmem.reflk
static int*
refcnt(uint64 pa)
{
  return kmem.refcnt + PPN(pa);
}

// pa need not be page aligned
void
changeref(uint64 pa, int increment)
{
  acquire(&kmem.reflk);
  *(refcnt(pa)) += increment;
  release(&kmem.reflk);
}

// static void
// setref(uint64 pa, int cnt)
// {
//   acquire(&kmem.reflk);
//   *(refcnt(pa)) = cnt;
//   release(&kmem.reflk);
// }


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.reflk, "reflock");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");


#ifndef LAB_SYSCALL
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
#endif
  
  r = (struct run*)pa;

  acquire(&kmem.reflk);
  if (--(*refcnt((uint64)pa)) <= 0) {
    *refcnt((uint64)pa) = 0;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&kmem.reflk);
}



// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.reflk);
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    debugassert(*refcnt((uint64)r) == 0, "refcnt not zero, pa %p\n", (void*)r);
    *refcnt((uint64)r) = 1;     // 此时一定有一个进程在使用
    kmem.freelist = r->next;
  }
  release(&kmem.lock);
  release(&kmem.reflk);
#ifndef LAB_SYSCALL
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
#endif
  return (void*)r;
}

