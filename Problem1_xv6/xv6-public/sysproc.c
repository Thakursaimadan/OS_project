#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "signal.h"
#include "memlayout.h"




extern struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

#define MSGBUF_SIZE 128  


#define MSGSIZE 128  // or whatever size you've allocated for msgbuf


int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int sys_kill(void) {
    int pid, sig;

    if (argint(0, &pid) < 0)
        return -1;
    if (argint(1, &sig) < 0)
        return -1;

    return kill(pid, sig);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
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

int
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

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
int
sys_getyear(void)
{
	return 1975;
}
//sysproc.c

int
sys_msgsend(void)
{
    int pid;
    char *msg;
    if (argint(0, &pid) < 0 || argstr(1, &msg) < 0)
        return -1;
    return sendmessage(pid, msg);
}

int sys_msgrecv(void) {
    int pid;
    char *buf;

    // Get the first argument (pid)
    if (argint(0, &pid) < 0) {
        cprintf("msgrecv: failed to get pid argument\n");
        return -1;
    }

    // Get the second argument (buffer)
    if (argstr(1, &buf) < 0) {
        cprintf("msgrecv: failed to get buffer argument\n");
        return -1;
    }

    // Call receivemessage with both pid and buf
    return receivemessage(pid, buf);
}


// Helper function to find a process by PID
struct proc* find_proc(int pid) {
    struct proc *p;
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid && p->state != UNUSED) {
            release(&ptable.lock);
            return p;
        }
    }
    release(&ptable.lock);
    return 0;
}

// System call to stop a process
int sys_sigstop(void) {
    int pid;
    if (argint(0, &pid) < 0)
        return -1;

    struct proc *p = find_proc(pid);
    if (!p)
        return -1;

    acquire(&ptable.lock);
    p->stopped = 1;  // Mark process as stopped
    release(&ptable.lock);
    return 0;
}

// System call to continue a stopped process
int sys_sigcont(void) {
    int pid;
    if (argint(0, &pid) < 0)
        return -1;

    struct proc *p = find_proc(pid);
    if (!p)
        return -1;

    acquire(&ptable.lock);
    p->stopped = 0;  // Clear stopped flag
    release(&ptable.lock);
    return 0;
}






// Inside sysproc.c

volatile unsigned int mutex = 0; // Global mutex variable (simple binary lock)

int sys_mutex_lock(void)
{
    // Spin until the lock is acquired (basic spinlock)
    while (xchg(&mutex, 1) != 0)
        ; // wait
    return 0;
}

int sys_mutex_unlock(void)
{
    // set mutex back to unlocked
    mutex = 0;
    return 0;
}




int
sys_clone(void)
{
    void (*function)(void*);
    void *arg;
    void *stack;

    if (argptr(0, (void*)&function, sizeof(function)) < 0 ||
        argptr(1, (void*)&arg, sizeof(arg)) < 0 ||
        argptr(2, (void*)&stack, sizeof(stack)) < 0)
        return -1;

    return clone(function, arg, stack);
}

int
sys_join(void)
{
    int tid;
    void **stack;

    if (argint(0, &tid) < 0 || argptr(1, (void*)&stack, sizeof(stack)) < 0)
        return -1;

    return join(tid, stack);
}