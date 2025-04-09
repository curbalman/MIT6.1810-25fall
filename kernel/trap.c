/* Syscall control flow
  |----usyscall()----|
  |                  |
  |ecall             |
  ↓                  |
uservec()       userret()
  |               ↑
  ↓               |
usertrap() --> usertrapret()
  | ↑
  ↓ |
syscall()
  | ↑
  ↓ |
syscalls[a7]()
*/




/* Where does everything goes?
let p = myproc()

                   arg0                  pc             return value
usyscall            a0      
uservec       p->trapframe->a0
usertrap                          p->trapframe->epc
syscall                                               p->trapframe->a0
usertrapret                       p->trapframe->epc
userret
usyscall
*/

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "debug.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();
void alarm_update();
void cow_handler();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8) {
    // system call

    if(killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
    //idebugf("syscall() return\n");
  } // Store/AMO page fault, is it COW?
  else if(r_scause() == 15) {
    cow_handler();  // note that cow_handler might never return
  }
  // this is a timer interrupt or device interrupt
  else if((which_dev = devintr()) != 0) {
    if(which_dev == 2) {
      alarm_update();
      // give up the CPU if this is a timer interrupt.
      yield();
    } else if (which_dev == 1) {

    } else {
      panic("usertrap: control should not reach here\n");
    }
  } else {
    goto unexpected;
  }

  if(killed(p))  exit(-1);
  usertrapret();
  panic("usertrap: control should not reach here\n");

unexpected:
  printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
  printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
  panic("usertrap(): unexpected scause\n");
  setkilled(p);
  if(killed(p))  exit(-1);
  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}

void
alarm_update()
{
  struct proc *p = myproc();
  if (p->alarm_enabled && p->ticks > 0 ) {
    int *passed = &(p->tickspassed);
    // passed++; 如果时间到了，就调用handler
    if ((*passed)++ >= p->ticks) {
      // alarm!
      *passed = 0;
      if (p->handler == MAXVA)  panic("usertrap: invalid handler address");
      p->alarm_enabled = 0;             // disable alarm, prevent reentrant alarm calls
                                        // 例如，在handler执行的时候，时间又到了，handler会被再次调用 
      p->sigframe = *(p->trapframe);    // 保存interrupt发生时的状态  
      p->trapframe->epc = p->handler;   // jump to handler function in user space
    }
  }
}

void 
cow_handler()
{
  uint64 va, newpa;
  pte_t *pte;
  uint64 flags;
  struct proc *p = myproc();

  va = r_stval();
  pte = walk(p->pagetable, va, 0);
  if (*pte == 0)  panic("cow_handler: null pte\n");
  // if not writeable, kill the process
  if (!(*pte & PTE_COW))  goto not_writeable;
  idebugf("cow page!!! va 0x%lx\nold pte: ", va);
  debugdo(print_pte, pte);
  va = PGROUNDDOWN(va);
  // kalloc new pape, memmove, map new page w/ PTE_W and w/o PTE_COW
  if((newpa = (uint64)kalloc()) == 0) {
    printf("cow_handler(): kalloc for cow page failed, killing process\n");
    goto kill;
  }

  // set PTE_W, clear PTE_COW
  flags = PTE_FLAGS(*pte);
  flags |= PTE_W;
  flags &= (~PTE_COW);

  memmove((void*)newpa, (char*)PTE2PA(*pte), PGSIZE);
  uvmunmap(p->pagetable, va, 1, 1);
  if(mappages(p->pagetable, va, PGSIZE, newpa, flags) != 0) {
    kfree((void*)newpa);
    printf("cow_handler(): mappages failed\n");
    goto kill;
  }
  idebugf("new pte: ");
  pte_t *newpte = walk(p->pagetable, va, 0);
  debugdo(print_pte, newpte);
  return;

not_writeable:
  printf("cow_handler(): not a cow page scause 0x%lx pid=%d\n", r_scause(), p->pid);
  printf("               sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
  printf("               write to read-only page?\n");
kill:
  setkilled(p);
  if(killed(p))  exit(-1);
  usertrapret();
}
