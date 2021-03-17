#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"

struct pTable{
  int head;
  int tail;   
  int size;
  struct spinlock lock;
  struct proc proc[NPROC];
  int order[NPROC];
} ;

struct proc_queue{
  int head;
  int tail;   
  int size;
  struct spinlock lock;
  struct proc proc[NPROC];
};

struct pTable ptable;
struct pstat pstat_table;
// struct proc_queue pqueue;



struct proc*
enqueue() {// need to lock while enqueing
  int i;
  for (i = 0; i < NPROC; i++) {
    if(ptable.proc[i].state == UNUSED)
      break;
  }
  if(i == NPROC){
    panic("No space in ptable");	  
  }
  struct proc* procintable = &(ptable.proc[i]);
  procintable->pstat_index=i;
  //  if(ptable.tail>=NPROC){
  //    panic("here");
  //  }
 
  ptable.order[ptable.tail] = i;
  ptable.tail = (ptable.tail + 1) % NPROC;
  ptable.size++;
  return procintable;
}

// TODO: TEST THIS
struct proc
dequeue() {
  // if(ptable.size > 1){
  //   ptable.head = (ptable.head + 1) % NPROC;
  // }
    if(ptable.size>=NPROC){
    panic("why");
  }
  if(ptable.order[ptable.head]>=NPROC){
    panic("here");
  }
  struct proc next_in_queue = ptable.proc[ptable.order[ptable.head]];
  if(ptable.size <=0){
    panic("here in not possible");
  }
  else{
        // if((ptable.head + 1) % NPROC==0){
        //   if(ptable.head==0)
        //   panic("efgefg");
        // }
    // if(ptable.head==NPROC-1){
    //   panic("makes sense");
    // }
    
    ptable.proc[ptable.order[(ptable.head)] ].state = UNUSED;
    pstat_table.inuse[ptable.order[(ptable.head)]]=0;
    pstat_table.compticks[ptable.order[(ptable.head)]]=0;
    ptable.head = (ptable.head + 1) % NPROC;
    // if(ptable.head==0){
    //   // if(ptable.size-10>=NPROC)
    //   panic("how os this possible");
    // }
    ptable.size--;
     // mod arithmetic may be sus
    // ptable.proc[(ptable.head - 1 + NPROC) % NPROC ].state = UNUSED;

    return next_in_queue;
  }
  
}

int enqueue_dequeue() {
  if(ptable.size == 0){
    return -1;
  }
  ptable.order[ptable.tail] = ptable.order[ptable.head];
  ptable.tail = (ptable.tail + 1) % NPROC;
  ptable.head = (ptable.head + 1) % NPROC;
  return 0;
}
struct proc*
peek() {
  return &ptable.proc[ptable.order[ptable.head]];
}

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

int setslice(int pid,int slice){
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid==pid){
      p->time_slice=slice;
      return 1;
    }
  // return 0;
}
  return -1;
}

int getpinfo(struct pstat *stat){
  // if(&pstat_table){
    if(stat==0){
      return -1;
    }
    *stat=pstat_table;
    return 0;
  
}

int getslice(int pid){
  struct proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid==pid){
      // p->time_slice=slice;
      return p->time_slice;
    }

}
return -1;
}
// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  // struct proc *test_p;
  char *sp;

  acquire(&ptable.lock);

  // for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  p=enqueue();
  if(p->state == UNUSED){
  // panic("here");
    goto found;}
  else{
    release(&ptable.lock);
    return 0;
  }
 // release(&ptable.lock);
//  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // acquire(&pqueue.lock);
  // test_p=enqueue();
  
  

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  p->time_slice=1;
  p->time_remaining=p->time_slice;
  p->time_assigned=p->time_remaining;
  pstat_table.inuse[p->pstat_index]=1;
  pstat_table.pid[p->pstat_index]=p->pid;
  pstat_table.timeslice[p->pstat_index]=p->time_slice;
  pstat_table.compticks[p->pstat_index]=0;
  pstat_table.schedticks[p->pstat_index]=0;
  pstat_table.switches[p->pstat_index]=0;
  pstat_table.sleepticks[p->pstat_index]=0;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}


int fork(){
  int slice=getslice(myproc()->pid);
  if(slice<0){
    return -1;
  }
  else{
    return fork2(slice);
  }
  
}
// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork2(int slice)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  np->time_slice=slice;
  pstat_table.timeslice[np->pstat_index]=np->time_slice;
  np->time_remaining=np->time_slice;
  np->time_assigned=np->time_remaining;

  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }
  dequeue();

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        pstat_table.inuse[p->pstat_index]=0;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->compensation_ticks=0;
        pstat_table.compticks[p->pstat_index]=0;
        p->time_assigned=0;
        p->current_ticks=0;
        p->sleep_period=0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int count=0;
  for(;;){
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

    for(int i = 0; i < ptable.size; i++){
    	p = peek();
	
	if (p->state!=RUNNABLE || p->time_remaining==0|| p->killed==1) {
    if(p->state==SLEEPING){
      count++;
    }
    if(p->state==RUNNABLE && p->time_remaining==0){
      p->time_remaining=p->time_slice;
      p->time_assigned=p->time_remaining;
       // not sure about switch need to discuss
    }
      if(p->killed==0){
        enqueue_dequeue();
        }// enqueue does not take any arguments?? how to enqueue a process?    
    	else{
        dequeue();
      }
      }  
        
        else if(p->state==RUNNABLE && p->time_remaining!=0){
        	c->proc = p;
          switchuvm(p);
          if(p->time_assigned==p->time_remaining){
            pstat_table.switches[p->pstat_index]++;
          }
          if((p->time_assigned-p->time_slice) >= (p->time_remaining)){
            pstat_table.compticks[p->pstat_index]++;
          }
          p->time_remaining=p->time_remaining-1;
          pstat_table.schedticks[p->pstat_index]++;

          p->state = RUNNING;
          swtch(&(c->scheduler), p->context);
          switchkvm();
          c->proc = 0;
	}
    }

    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == SLEEPING){
      pstat_table.wakeup_compensation[p->pstat_index]++;
    }
    if(p->state == SLEEPING && p->chan == chan){
    if(chan==&ticks){
      // if(holding())
      // acquire(&tickslock);
      
      p->compensation_ticks++;
      if((p->current_ticks+p->sleep_period)==ticks){
        p->state=RUNNABLE;
        p->current_ticks=0;
        p->sleep_period=0;
        p->time_remaining=p->time_slice+p->compensation_ticks;
        p->time_assigned=p->time_slice+p->compensation_ticks;
        pstat_table.compensation[p->pstat_index]=p->compensation_ticks;
        p->compensation_ticks=0;
      }
      
      // release(&tickslock);
    }
    else{
      p->state = RUNNABLE;
    }
    }
}

}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
