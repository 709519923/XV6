I need to do a conclusion about page table. I think I gradually learn how the code work in terms of forming a OS.~
@[TOC]

1. where is page table?
2. function of page table?
3. how to get arguments from user to kernel? -- 
**reference**:[https://stackoverflow.com/questions/46870509/how-to-pass-a-value-into-system-call-xv6](https://stackoverflow.com/questions/46870509/how-to-pass-a-value-into-system-call-xv6) 
# 1.speed up system call
Code learning:
![在这里插入图片描述](https://img-blog.csdnimg.cn/4cbfc878bbbe407fb543e7d6f6cb9c20.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/ca8d3980447f48a8aab0fcf34f0f135e.png)
according to hint, change the code step by step:
- **<font color="#67C23A">You can perform the mapping in proc_pagetable() in kernel/proc.c.</font>**
- **<font color="#67C23A">Choose permission bits that allow userspace to only read the page.</font>**
- **<font color="#67C23A">You may find that mappages() is a useful utility.</font>**
```c
//proc.c
  // map the trapframe just below trapframe, for read-only page mapping
  if(mappages(pagetable, USYSCALL, PGSIZE,
              (uint64)(p->usyscall), PTE_R | PTE_U) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmunmap(pagetable, USYSCALL, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }
```



- **<font color="#67C23A">Don't forget to allocate and initialize the page in allocproc().</font>**

```c
...//proc.c
	found:
  // Allocate a pid page.  'allocate and initialize the page'
  if((p->usyscall = (struct usyscall *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  p->usyscall->pid = p->pid;
```


**<font color="#67C23A">Make sure to free the page in freeproc().</font>**

```c
//proc.c
// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
...
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->usyscall)
    kfree((void*)p->usyscall);
  p->usyscall = 0;
 ...
```

```c
//proc.c
// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmunmap(pagetable, USYSCALL, 1, 0);

  uvmfree(pagetable, sz);
}
```


# 2. print page
>You can put vmprint() in kernel/vm.c.

```c
int
exec(char *path, char **argv)
{
...
  if(p->pid==1){
    vmprint(p->pagetable);
  }
return argc
```

>Use the macros at the end of the file kernel/riscv.h.
The function `freewalk` may be inspirational.
Define the prototype for vmprint in kernel/defs.h so that you can call it from exec.c.
Use %p in your printf calls to print out full 64-bit hex PTEs and addresses as shown in the example.


```c
// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}
```

```c
// Recursively check valid page-table pages.
void
vmprint(pagetable_t pagetable, int depth)
{
  char* buf = "";
  switch (depth)
  {
  case 0:
    printf("page table %p\n", pagetable);
    buf = "..";
    break;
  case 1:
    buf = ".. ..";
    break;
  case 2:
    buf = ".. .. ..";
    break;
  }
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if(pte & PTE_V){
      // ..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      printf("%s%d: pte %p pa %p\n", buf, i, pte, child);
      if(depth != 2)
      vmprint((pagetable_t)child, ++depth);
    }
  }
}
```


# 3. Detecting which pages have been accessed
Look at the user program, found that it uses a buffer.
- buffer has 32 pages.
- For 1,2,30 page, we set 1 as they've been accessed.
- we call the `pageccess()`, store the access result in the abits, then compare `abits` and assumption.  
```c
pgaccess_test()
{
  char *buf;
  unsigned int abits;
  printf("pgaccess_test starting\n");
  testname = "pgaccess_test";
  buf = malloc(32 * PGSIZE);
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");
  buf[PGSIZE * 1] += 1;
  buf[PGSIZE * 2] += 1;
  buf[PGSIZE * 30] += 1;
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");
  if (abits != ((1 << 1) | (1 << 2) | (1 << 30)))
    err("incorrect access bits set");
  free(buf);
  printf("pgaccess_test: OK\n");
}

```

>You'll need to parse arguments using argaddr() and argint().

we see the two functions below, found that they can get arguments from `pgaccess()` by the kernel.
the way of getting arguments is access to `trapframe` register.

```c
// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}
```
So, we use them to input arguments from user program.

```c
  uint64 *buffer;
  int pg_amount;
  uint64 *abits
  if(argaddr(0, %buffer) < 0) return -1;
  if(argint(1, &pg_amount) < 0) return -1;
  if(argaddr(2, abits) < 0) return -1;
```

result:

```bash
abits = 0x0000000000000000
pgtbltest: pgaccess_test failed: incorrect access bits set, pid=3
```
>walk() in kernel/vm.c is very useful for finding the right PTEs.

check the `walk()` function, found that it can return the PTE's address if valid. So, we use it for finding valid PTE
```c
// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}
```

```c
//vm.c
//check accessed page
//return uint 32 mask for showing every page's condition of access
uint32
check_page(pagetable_t pagetable, uint64 start_va, int pg_amount)
{
  if(start_va >= MAXVA)
    panic("walk");
  uint32 mask = 0;
  for(int i = 0; i < pg_amount; i++){
    pte_t * pte = walk(pagetable, start_va + i * PGSIZE, 0);
    if(*pte & PTE_A) {
      printf("i = %d\t", i);
      mask |= 1 << i;
      printf("mask = %p\n", mask);
      *pte &=  ~PTE_A; // 0 << 6 for bit clear
    }
  }
  return mask;
}
```
