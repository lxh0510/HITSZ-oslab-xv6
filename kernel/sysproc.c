#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
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

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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

//trace
uint64
sys_trace(void)
{
  int mask;
  if(argint(0,&mask)<0)             //若读取mask异常返回-1
    return -1;
  myproc()->mask = mask;                  //将读取的mask存入结构体proc中
  return 0;                               //正常返回0
}

//sysinfo
uint64
sys_sysinfo(void)
{
  struct sysinfo info;                  //创建sysinfo结构体
  info.freemem = freemem();             //填写结构体各个成员变量
  info.nproc = nproc();
  info.freefd = freefd(); 
  uint64 addr;
  if(argaddr(0,&addr)<0)             //获得用户进程页表地址
    return -1;
  if(copyout(myproc()->pagetable, addr, (char *)&info, sizeof(info)) < 0)    //将内核空间中结构体复制到用户空间
      return -1;
  return 0;
}