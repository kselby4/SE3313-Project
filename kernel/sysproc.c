#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "ecostat.h"

extern uint ticks;
extern struct proc proc[];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;
  struct proc *p = myproc();

  argint(0, &n);
  if(n < 0)
    n = 0;

  acquire(&tickslock);

  acquire(&p->lock);
  if(p->eco_enabled && p->eco_period > 0){
    release(&p->lock);
    while(ticks < p->eco_wake_tick){
      if(killed(p)){
        release(&tickslock);
        return -1;
      }
      sleep(&ticks, &tickslock);
    }
    acquire(&p->lock);
    p->eco_wake_tick = ticks + p->eco_period;
    release(&p->lock);
    release(&tickslock);
    return 0;
  }
  release(&p->lock);

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

uint64
sys_setecoperiod(void)
{
  int n;
  struct proc *p = myproc();
  uint t;

  argint(0, &n);

  if(n <= 0){
    acquire(&p->lock);
    p->eco_enabled = 0;
    p->eco_period = 0;
    p->eco_wake_tick = 0;
    release(&p->lock);
  } else {
    acquire(&tickslock);
    t = ticks;
    release(&tickslock);
    acquire(&p->lock);
    p->eco_enabled = 1;
    p->eco_period = n;
    p->eco_wake_tick = t + n;
    release(&p->lock);
  }
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
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
sys_kps(void)
{
  char buf[4]; // enough for "-o" or "-l" + '\0'

  if(argstr(0, buf, sizeof(buf)) < 0)
    return -1;

  return kps(buf);
}

uint64
sys_setschedmode(void)
{
  int mode;
  argint(0, &mode);
  return set_sched_mode(mode);
}

uint64
sys_getschedmode(void)
{
  return get_sched_mode();
}

uint64
sys_getecostats(void)
{
  uint64 ubuf;
  int max;
  struct ecostat tmp[NPROC];
  struct proc *p;
  struct proc *cur = myproc();
  int n = 0;

  argaddr(0, &ubuf);
  argint(1, &max);
  if(max < 0)
    return -1;
  if(max > NPROC)
    max = NPROC;
  if(max == 0)
    return 0;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state != UNUSED && n < max){
      tmp[n].pid = p->pid;
      tmp[n].state = p->state;
      memmove(tmp[n].name, p->name, sizeof(tmp[n].name));
      tmp[n].running_ticks = (int)p->running_ticks;
      tmp[n].runnable_ticks = (int)p->runnable_ticks;
      tmp[n].sleeping_ticks = (int)p->sleeping_ticks;
      tmp[n].context_switches = (int)p->context_switches;
      tmp[n].eco_score = proc_energy_score(p);
      n++;
    }
    release(&p->lock);
  }

  if(n > 0 && copyout(cur->pagetable, ubuf, (char *)tmp, n * sizeof(struct ecostat)) < 0)
    return -1;
  return n;
}

