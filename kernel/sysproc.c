#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

// return 0 if success, -1 if fail(due to calling process early terminated)
uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgpte(void)
{
  uint64 va;
  struct proc *p;  

  p = myproc();
  argaddr(0, &va);
  pte_t *pte = pgpte(p->pagetable, va);
  if(pte != 0) {
      return (uint64) *pte;
  }
  return 0;
}
#endif

#ifdef LAB_PGTBL
// print process patbl
int
sys_kpgtbl(void)
{
  struct proc *p;  

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif


uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;
  struct proc *p = myproc();

  argint(0, &mask);
  if (mask <= 0)  return -1;
  p->tracemask = mask;
  printf("trace mask set\n");
  return 0;
}


uint64
sys_sigalarm(void)
{
  // int sigalarm(int ticks, void (*handler)())
  int ticks;
  uint64 handler;
  struct proc* p = myproc();

  argint(0, &ticks);
  argaddr(1, &handler);
  if (ticks) {
    p->alarm_enabled = 1;
    p->ticks = ticks;
    p->handler = handler;
  }
  else
    p->alarm_enabled = 0;
  return 0;
}

uint64
sys_sigreturn()
{
  struct proc* p = myproc();
  p->alarm_enabled = 1;     // turn on alarm
  // 回到interrupt发生时的状态 
  *(p->trapframe) = p->sigframe;
  return p->sigframe.a0;      // restore previous a0, userret set register a0 to this value
}
