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

int refcnts[NPHYPG];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// add/sub refcnt of addr pa
// return the new refcnt
// pa need not be page-aligned
int
setref(void *pa, int cnt)
{
  if( (char*)pa < end || (uint64)pa >= PHYSTOP ) panic("setref: invalid address");
  if(cnt < 0) panic("setref: attempt to set a negative refcnt");
  return refcnts[PPX(pa)] = cnt;
}
int
getref(void *pa)
{
  return refcnts[PPX(pa)];
}
int
incrref(void *pa)
{
  return setref(pa, getref(pa) + 1);
}
int
decrref(void *pa)
{
  return setref(pa, getref(pa) - 1);
}


// Free physical memory but DOES NOT check refcnt
static void
dokfree(void *pa)
{
  struct run *r;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// same as freerange but DOES NOT check refcnt
static void
dofreerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    dokfree(p);
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // freerange(end, (void*)PHYSTOP);
  dofreerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  // printf("kfree: %p...", pa);
  // do not free the page when there are still referencts to it
  if ( getref(pa) == 0 ) {
    // no reference, free the memory
    dokfree(pa);
    // printf("freed\n");
  } else {
    // printf("no free\n");
  }
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

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    setref(r, 1);
    // printf("kalloc: %p\n", (void*)r);
  }
  return (void*)r;
}
