// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define NNAMES 8

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

char *locknames[NNAMES] = {
  "kmem0",
  "kmem1",
  "kmem2",
  "kmem3",
  "kmem4",
  "kmem5",
  "kmem6",
  "kmem7",
};

void
kinit()
{
  if ( NNAMES < NCPU ) panic("kinit");
  for(int i = 0; i < NCPU; ++i) {
    initlock(&(kmem[i].lock), locknames[i]);
  }
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

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int myid;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  myid = cpuid();
  acquire(&(kmem[myid].lock));
  r->next = kmem[myid].freelist;
  kmem[myid].freelist = r;
  release(&(kmem[myid].lock));
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  // TODO：
  // 1. 每次要偷页表时，不都从CPU0开始
  // 2. 一次偷多几个页表
  struct run *r;
  int myid;

  push_off();
  myid = cpuid();
  acquire(&(kmem[myid].lock));
  r = kmem[myid].freelist;
  if(r) {
    kmem[myid].freelist = r->next;
    release(&(kmem[myid].lock));
  }
  else {  // steal
    release(&(kmem[myid].lock));
    for (int i = 0; i < NCPU; ++i) {
      if ( i == myid ) continue;
      acquire(&(kmem[i].lock));
      struct run *ri = kmem[i].freelist;
      if (ri) {
        r = ri;
        kmem[i].freelist = ri->next;
        release(&(kmem[i].lock));
        break;
      }
      release(&(kmem[i].lock));
    }
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
