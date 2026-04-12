#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
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
sys_memsize(void)
{
  return myproc()->sz;
}

uint64
sys_co_yield(void)
{
  int target_pid, value;
  struct proc *p = myproc();
  struct proc *target_p = 0;
  extern struct proc proc[NPROC];
  int return_val;

  // (1) Extract arguments
  argint(0, &target_pid);
  argint(1, &value);

  if(target_pid <= 0 || target_pid == p->pid)
    return -1;

  // (2) Search loop - Skip self to avoid 'panic: acquire'
  for(struct proc *tp = proc; tp < &proc[NPROC]; tp++) {
    if(tp == p) continue; 
    acquire(&tp->lock);
    if(tp->pid == target_pid) {
      if(tp->state != UNUSED && !tp->killed) {
        target_p = tp;
        break; 
      }
    }
    release(&tp->lock);
  }

  if(target_p == 0)
    return -1;

  acquire(&p->lock); 

  if (target_p->state == SLEEPING && target_p->chan == (void*)p) {
    // --- SCENARIO A: Target is waiting for us (Orange case) ---
    
    // Save the value the target left for us
    return_val = target_p->trapframe->a0;
    
    // Inject our value into target's return register
    target_p->trapframe->a0 = value;
    
    // Update states for direct handoff
    target_p->state = RUNNING;
    target_p->chan = 0;
    p->state = SLEEPING;
    p->chan = (void*)target_p;

    // DIRECT SWITCH PROTOCOL:
    // We hold BOTH p->lock and target_p->lock.
    // According to xv6, we switch and the target will release its own lock.
    swtch(&p->context, &target_p->context);

    // --- RESUME HERE ---
    // When we wake up, someone else yielded back to us.
    // They left p->lock held for us.
    p->chan = 0;
    
    // CRITICAL: We must also release the lock of the process that switched to us
    release(&target_p->lock); 
    release(&p->lock);
    
    return return_val;

  } else {
    // --- SCENARIO B: Target not ready (Green case) ---
    
    p->trapframe->a0 = value; // Leave our value in our register
    p->state = SLEEPING;
    p->chan = (void*)target_p;

    release(&target_p->lock); // Release target before sched()
    
    sched(); // Switch to scheduler

    // --- RESUME HERE ---
    p->chan = 0;
    return_val = p->trapframe->a0; // The value that was injected into us
    release(&p->lock);
    
    return return_val;
  }
}