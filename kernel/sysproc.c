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

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
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
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 vm;        // 要检查的用户页的第一个虚拟地址
  int num_page;     // 要检查的用户页的数目
  uint64 dst_buffer;  // 结果要保存的buffer地址
  argaddr(0,&vm);
  argint(1, &num_page);
  argaddr(2,&dst_buffer);
  int ans=0;
  uint64 va=vm;
  for(int i=0;i<((num_page<32)?num_page:32);i++,va+=PGSIZE)
  {
    pte_t* pte=walk(myproc()->pagetable, va, 0);
    if((*pte) & PTE_A)
      ans |=(1<<i);
    (*pte) &= (~(uint64)PTE_A);
  }
  copyout(myproc()->pagetable,dst_buffer,(char*)&ans,sizeof(int));
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
