
# trace
Reference
-   [https://www.geeksforgeeks.org/xv6-operating-system-adding-a-new-system-call/#discuss](https://www.geeksforgeeks.org/xv6-operating-system-adding-a-new-system-call/#discuss)
to show u how the 5 files work when adding a system call.
after configuring, I got a easy result below.
- [https://miaochenlu.github.io/2020/12/16/xv6-lab2/](https://miaochenlu.github.io/2020/12/16/xv6-lab2/)
ZJUer for the whole lab.
- [https://medium.com/@mahi12/adding-system-call-in-xv6-a5468ce1b463](https://medium.com/@mahi12/adding-system-call-in-xv6-a5468ce1b463)
explanation of how code works.
[https://github.com/whileskies/xv6-labs-2020/blob/main/doc/Lab2-system%20calls.md](https://github.com/whileskies/xv6-labs-2020/blob/main/doc/Lab2-system%20calls.md)
github blog.
```
$ trace
Jason was born in year 1999
```
So, Now we can add more code and test `trace` function.
here are the files we need to modify:

write `sys_trace()` to input the mask:

```c
// for sys_trace definition
uint64
sys_trace(void)
{
  int mask;
  if(argint(0,&mask) < 0){  // transfer 'char' to 'int'
    return -1;
  }
  struct proc *p = myproc();
  p->mask = mask;
  printf("input mask == %d\n", mask);
  return 0;
}
```
this function can surely get `32` at `trace()` in the `trace.c`

```bash
$ trace 32 grep hello README
input mask == 32
6 :  syscall read 1023
6 :  syscall read 968
6 :  syscall read 235
6 :  syscall read 0
```

```c
//syscall.c add
static char *syscall_name[] = {
  "", "fork", "exit", "wait", "pipe", "read", "kill", "exec", "fstat", "chdir", "dup",
  "getpid", "sbrk", "sleep", "uptime", "open", "write", "mknod", "unlink", "link", "mkdir",
  "close", "trace"
};

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
    if(p->mask & 1 << num){  // AND operation, bit movement
      // syscall num == 1 AND mask == 1
      printf("%d :  syscall %s %d\n",
            p->pid, syscall_name[num], p->trapframe->a0);
      // [pid]: syscall [call_name] [return value]
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}

```
So far, trace function finish. but there is still one question. why `trace()` has no implementation but still work?
As I know, I've not written it. So, I go back to lab1, and see the hint:

 - See kernel/sysproc.c for the xv6 kernel code that implements the sleep system call (look for sys_sleep), user/user.h for the C definition of sleep callable from a user program, and user/usys.S for the assembler code that jumps from user code into the kernel for sleep.

```c
.global fork
fork:
 li a7, SYS_fork
 ecall
 ret
```
this assembly means `fork()` can trigger `SYS_fork` pointer address, then the address->a7, `sys_fork` therefore run in a privileged state.


# sysinfo
first run after basic configuration:
```c
$ sysinfotest
sysinfotest: start
FAIL: sysinfo succeeded with bad argument
```
Now, we need to modify the `sys_sysinfo` to get system's free memory & process amount.

```c
//sysproc.c
// give system information: free memory & process
// info_pointer is a user virtual address, pointing to a struct sysinfo.
uint64
sys_sysinfo(void)
{
  uint64 info_pointer;
  struct proc *p = myproc();
  struct sysinfo info;
  info.freemem = freemem();
  info.nproc = nproc();
  argaddr(0, &info_pointer);
  if(copyout(p->pagetable, info_pointer, (char *)&info, sizeof(info)) < 0)
      return -1;
  return 0;
}
```
but, `freemem()` and `nproc()` haven't been implemented.

```c
// kalloc.c
uint64
freemem(void)
{
  struct run *r;
  uint64 count = 0;
  acquire(&kmem.lock);
  r = kmem.freelist;
  while(r){
    count++;
    r = r->next;
  }
  release(&kmem.lock);
  return PGSIZE * count;
}

```

```c
//proc.c
// nproc
uint64
nproc(void)
{
  struct proc *p;
  uint64 count = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state != UNUSED) {
      count++;
    }
    release(&p->lock);
  }
  return count;
}
```










- reference of writting `nproc` & `freemem`
```c
// reference of writting nproc
// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}
```

```c
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

```

result:(adding test code in sysinfortest.c, we can see free memory & process number)

```bash
$ sysinfotest
sysinfotest: start
free memory = 133128192
num of process = 3
sysinfotest: OK
```
note for review:
lab1 tells user programm
lab2 tells kernel, there's some relation between them, try to compare.
