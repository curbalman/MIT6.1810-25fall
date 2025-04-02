// https://www.cnblogs.com/nosae/p/18615965 https://nos-ae.github.io/posts/attack-xv6/
// https://blog.csdn.net/weixin_42543071/article/details/143351746

/* attacktest 使用物理页分析：
1. fork() （+10页）
    - 调用allocproc来创建struct proc
      - （+1页）allocproc中首先使用kalloc为p->trapframe分配一页物理页
      - （+1页）接着调用proc_pagetable创建页表，其中，先调用uvmcreate为根页表分配一物理页
      - （+2页）把trampoline和trapframe映射到虚拟内存，为二、三级页表分别分配一页物理页
    - 用uvmcopy复制父进程attacktest的用户内存（不包括trapframe, trampoline）
      - （+2页）由于用户内存在低地址区域，与trapfram相隔很远，故二、三级页表分别新建一页物理页
      - _attacktest用户内存有4页
        - （+2页）readelf -l user/_attacktest看到有两个load段
        - （+2页）用户栈一页，guard page一页
  故1+1+2+2+2+2一共分配10页物理页
2. exec() （+5页表 +4用户页 -9释放）
    - （+3页）调用proc_pagetable创建新页表，共需要三级页表来映射trapframe和trampoline
    - 调用uvmalloc为loadseg创造空间
      - （+2页）低地址内存，需要新的二三级页表
      - （+2页）secret的elf有两个load段readelf -l user/_secret
    - （+2页）分配用户栈一页，和guard page一页。栈是紧挨着data段和text段之后分配的，他们属于同一个三级页表，不需要额外分配页表
    - 调用proc_freepagetable来释放旧页表和旧用户内存
      - （0页）先unmap trampoline和trapframe，因为传给uvmunmap的参数do_free=0，所以不释放物理页
      - （-9页）释放之前fork()分配的物理页，但不包括trapframe
3. secret()
    分配32物理页，在第10页写入秘密

4. wait() -> freeproc()
    (注意释放顺序)
    - （-1页）释放trapframe
    - proc_freepagetable() -> uvmfree()
      - 调用uvmunmap从低到高地址释放用户内存
        - （-2页）data段、text段
        - （-2页）用户栈、grard page
        - （-32页）32页堆内存，从低地址页到高地址页依次释放/入栈
      - （-5页）freewalk() 释放页表
  故最后链表栈kmem从栈顶(最后放入)开始的页依次为：5页页表、第32页堆内存、第31页堆内存、…、第10页堆内存（密码所在的页）、…、第1页堆内存、page guard…..

5. attack()
  - （+10页）fork()，其中有5页来自5页页表，另外5页来自第32~28页堆内存
  - exec()
    - （+9页），此时kmem从栈顶依次为：第18页堆内存、第17页、...、第10页、...
    - （-9页），此时kmem从栈顶依次为：9页释放的、第18页堆内存、第17页、...、第10页、...
  故需要分配17

*/
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  // your code here.  you should write the secret to fd 2 using write
  // (e.g., write(2, secret, 8)
  char *end = sbrk(17*PGSIZE);

  // for debugging: 打印每一页的内容
  // for (int i = 0; i < 32; i++)
  // {
  //   printf("%d: ", i);
  //   char *p = end + i * PGSIZE;
  //   p += 32;
  //   for (int j = 0; j < 8; j++)
  //     printf("%d ", *(p+j));
  //   printf("\n");
  // }

  // TODO 为什么ulib.c大小会影响secret在第几页
  end += 16 * PGSIZE;
  write(2, end+32, 8);
  exit(1);
}
