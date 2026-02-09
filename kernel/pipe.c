#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

#define PETERSON_WRITER 0   /* Peterson process id for writer */
#define PETERSON_READER 1   /* Peterson process id for reader */

struct pipe {
  struct spinlock lock;       /* used only for sleep/wakeup (full or empty), not for buffer access */
  volatile int flag[2];       /* Peterson: flag[i]=1 means process i wants to enter critical section */
  volatile int turn;          /* Peterson: whose turn it is when both want in (0 or 1) */
  char data[PIPESIZE];        /* ring buffer for pipe data */
  uint nread;                 /* total bytes consumed (read index into ring) */
  uint nwrite;                /* total bytes written (write index into ring) */
  int readopen;               /* read fd still open */
  int writeopen;              /* write fd still open */
};

static inline void peterson_enter(struct pipe *pi, int id)  /* enter critical section; id: 0=writer, 1=reader */
{
  int other = 1 - id;                    /* the other process's id */
  pi->flag[id] = 1;                      /* say "I want to enter" */
  __sync_synchronize();                  /* memory barrier so other CPU sees flag[id] */
  pi->turn = other;                      /* yield to the other process first */
  __sync_synchronize();                  /* barrier again */
  while (pi->flag[other] == 1 && pi->turn == other)  /* wait while other wants in and it's their turn */
    ;
  __sync_synchronize();                  /* barrier before touching shared buffer */
}

static inline void peterson_leave(struct pipe *pi, int id)   /* leave critical section; release buffer access */
{
  __sync_synchronize();                  /* ensure our writes to buffer are visible */
  pi->flag[id] = 0;                      /* say "I am done" */
}

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  pi->flag[0] = 0;             /* Peterson: writer not in critical section */
  pi->flag[1] = 0;             /* Peterson: reader not in critical section */
  pi->turn = 0;                /* Peterson: initial turn value */
  initlock(&pi->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
  acquire(&pi->lock);
  if(writable){
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    kfree((char*)pi);
  } else
    release(&pi->lock);
}

int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();

  while(i < n){
    acquire(&pi->lock);                           /* need lock to check state and to sleep/wakeup */
    if(pi->readopen == 0 || killed(pr)){
      release(&pi->lock);
      return -1;
    }
    if(pi->nwrite == pi->nread + PIPESIZE){       /* pipe buffer is full */
      wakeup(&pi->nread);                         /* wake reader so it can drain */
      sleep(&pi->nwrite, &pi->lock);              /* sleep until reader frees space; releases lock */
      continue;
    }
    release(&pi->lock);                            /* release lock so reader can run; buffer access uses Peterson */

    char ch;
    if(copyin(pr->pagetable, &ch, addr + i, 1) == -1)  /* copy one byte from user space */
      break;
    peterson_enter(pi, PETERSON_WRITER);           /* enter critical section (writer = process 0) */
    pi->data[pi->nwrite++ % PIPESIZE] = ch;       /* put byte in ring buffer and advance write index */
    peterson_leave(pi, PETERSON_WRITER);          /* leave critical section */
    i++;
    acquire(&pi->lock);                           /* need lock to call wakeup */
    wakeup(&pi->nread);                           /* wake reader in case it was sleeping on empty */
    release(&pi->lock);
  }
  acquire(&pi->lock);
  wakeup(&pi->nread);                             /* final wakeup for any reader waiting on data */
  release(&pi->lock);

  return i;
}

int
piperead(struct pipe *pi, uint64 addr, int n)
{
  int i;
  struct proc *pr = myproc();
  char ch;

  acquire(&pi->lock);
  while(pi->nread == pi->nwrite && pi->writeopen){  /* pipe is empty and writer might still write */
    if(killed(pr)){
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock);                  /* sleep until writer adds data; releases lock */
  }
  release(&pi->lock);                              /* release lock; buffer access uses Peterson below */

  for(i = 0; i < n; i++){
    peterson_enter(pi, PETERSON_READER);           /* enter critical section (reader = process 1) */
    if(pi->nread == pi->nwrite){                  /* no data left after we got in */
      peterson_leave(pi, PETERSON_READER);
      break;
    }
    ch = pi->data[pi->nread % PIPESIZE];           /* take one byte from ring buffer */
    pi->nread++;                                  /* advance read index */
    peterson_leave(pi, PETERSON_READER);          /* leave critical section */
    if(copyout(pr->pagetable, addr + i, &ch, 1) == -1) {  /* copy byte to user space */
      if(i == 0)
        i = -1;
      break;
    }
    acquire(&pi->lock);                           /* need lock to call wakeup */
    wakeup(&pi->nwrite);                          /* wake writer in case it was sleeping on full */
    release(&pi->lock);
  }
  return i;
}
