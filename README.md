In this lab, we start to learn about some RISC-V instruction and related registers.
For these two parts, here is a video for warm up.
[https://www.youtube.com/watch?v=Z5JC9Ve1sfI&ab_channel=TomScott](https://www.youtube.com/watch?v=Z5JC9Ve1sfI&ab_channel=TomScott)
At the following code, we can see the assembly of RISC-V
`1c` : `pc` value
`1141`: unknown
`addi` : instruction
`sp,sp,-16`: instruction arguments
```c
//call.asm
void main(void) {
  1c:   1141                    addi    sp,sp,-16
  1e:   e406                    sd      ra,8(sp)
  20:   e022                    sd      s0,0(sp)
  22:   0800                    addi    s0,sp,16
  printf("%d %d\n", f(8)+1, 13);
  24:   4635                    li      a2,13
  26:   45b1                    li      a1,12
  28:   00000517                auipc   a0,0x0
  2c:   7b050513                addi    a0,a0,1968 # 7d8 <malloc+0xea>
  30:   00000097                auipc   ra,0x0
  34:   600080e7                jalr    1536(ra) # 630 <printf>
  exit(0);
  38:   4501                    li      a0,0
  3a:   00000097                auipc   ra,0x0
  3e:   27e080e7                jalr    638(ra) # 2b8 <exit>


```
[https://img-blog.csdnimg.cn/14b3356523ef4bffa25dfbfd78f2a2e1.png](https://img-blog.csdnimg.cn/14b3356523ef4bffa25dfbfd78f2a2e1.png)
1.a2
2.No, they are inline in the `main()`
3.0x30
4.0x30+$(1536)_{10}$ = 0x630
5.

```c
$ call
HE110 World$ 
```
[https://img-blog.csdnimg.cn/8074ed66e74f4a03b46912c6c2724855.png]
variable `i` need to be read in pair:
$64$ $6C$ $72$ , they are corresponding to character in ACSII.

6. value is always 1. But value of `a2` would be uncertain.

```c
HE110 Worldx=3 y=1$ call
HE110 Worldx=3 y=1$ call
HE110 Worldx=3 y=1$ call
```

```c
$ call
x=3 y=5221$ call
x=3 y=5221$ call
x=3 y=5221$ call
x=3 y=5221$ 
```

```c
 printf("x=%d y=%d", 3,5);
  4a:   4615                    li      a2,5
  4c:   458d                    li      a1,3
  4e:   00000517                auipc   a0,0x0
  52:   7d250513                addi    a0,a0,2002 # 820 <malloc+0xfa>
  56:   00000097                auipc   ra,0x0
  5a:   612080e7                jalr    1554(ra) # 668 <printf>
  printf("x=%d y=%d", 3);
  5e:   458d                    li      a1,3
  60:   00000517                auipc   a0,0x0
  64:   7c050513                addi    a0,a0,1984 # 820 <malloc+0xfa>
  68:   00000097                auipc   ra,0x0
  6c:   600080e7                jalr    1536(ra) # 668 <printf>
```


# Backtrace
add the backtrace function following the hint:

```c
//printf.c
void
backtrace(void)
{
  uint64 framepointer = r_fp();
  printf("%p\n", framepointer);
}
```
result:
```bash
$ bttest
0x0000003fffff9f80
$ bttest
0x0000003fffff9f80
```
>hint:
>These lecture notes have a picture of the layout of stack frames. Note that the return address lives at a fixed offset (-8) from the frame pointer of a stackframe, and that the saved frame pointer lives at fixed offset (-16) from the frame pointer.

pointer's ADD operation is shown below. first, a `*ptr` pointer needed to be defined.
than, use the `ptr` to for address movement.
So, for a uint64 pointer` uint64 framepointer = r_fp();`

```c
#include <stdio.h>

const int MAX = 3;

int main () {

   int  var[] = {10, 100, 200};
  int  i, *ptr; // attention!

   /* let us have array address in pointer */
   ptr = var;
	
   for ( i = 0; i < MAX; i++) {

      printf("Address of var[%d] = %x\n", i, ptr );
      printf("Value of var[%d] = %d\n", i, *ptr );

      /* move to the next location */
      ptr++;  // attention!
   }
	
   return 0;
}
```
address operation need a pointer variable
`printf("%p\n", *(framepointer-8));` this doesn't work.
**There is no way to avoid using pointers when operations involving addresses offset.**
For example, knowing an address and want to read the value of the next address.
`framepointer` is the `uint64`
`framepointer-8` is also a `uint64`
But we want to get the value stored in the `framepointer - 8`
if `framepointer-8` is a pointer, easy, use `*` to dereference.
So, we use **Type Cast** to make a `uint64 pointer`, that is 
`(uint64*) (framepointer -8)` 
to get the value inside, dereference it.
So we can print the value like this:
`*(uint64 *)(framepointer  - 8)` :laughing::laughing::laughing:

```c
//improved
void
backtrace(void)
{
  uint64 framepointer = r_fp(),ra;
  uint64 top = PGROUNDDOWN(framepointer);
  uint64 btm = PGROUNDUP(framepointer);
  // printf("%p\n", btm);
  // printf("%p\n", top);
  while (framepointer < btm && framepointer > top)
  {
    ra = *(uint64 *)(framepointer - 8);
    framepointer = *(uint64 *)(framepointer - 16);
    printf("%p\n", ra);
  }
  //  stack 
}
```
![在这里插入图片描述](https://img-blog.csdnimg.cn/da349e007fb04abdbfd14216f37738a0.png)
the address is corresponding to code line in the file `sysproc.c` `syscall.c` `trap.c` 
```c
$ bttest
0x0000000080002144
0x0000000080001fa6
0x0000000080001c90
$ QEMU: Terminated
jason@jason-virtual-machine:~/Desktop/traps/xv6-labs-2021$ addr2line -e kernel/kernel
0x0000000080002144
/home/jason/Desktop/traps/xv6-labs-2021/kernel/sysproc.c:76
0x0000000080001fa6
/home/jason/Desktop/traps/xv6-labs-2021/kernel/syscall.c:140
0x0000000080001c90
/home/jason/Desktop/traps/xv6-labs-2021/kernel/trap.c:76
```
modify `panc` function in `printf.c`

```c
void
panic(char *s)
{
  pr.locking = 0;
  printf("panic: ");
  printf(s);
  printf("\n");
  backtrace(); // this line ___
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}
```
# Alarm

> In this exercise you'll add a feature to xv6 that periodically alerts a process as it uses CPU time. This might be **useful for compute-bound processes** that want to limit how much CPU time they chew up, or for processes that want to compute but also want to **take some periodic action**. More generally, you'll be implementing a primitive form of u**ser-level interrupt/fault handlers**; you could use something similar to handle page faults in the application, for example.

[C function as argument in function
](https://blog.csdn.net/qq_44733706/article/details/100799773)

- add the system function

```c
//user.h
// system calls

int sigalarm(int interval, void (*handler)());
int sigreturn(void);

```

```c
//syscall.c
extern uint64 sys_sigalarm(void);
extern uint64 sys_sigreturn(void);

static uint64 (*syscalls[])(void) = {

[SYS_sigalarm]  sys_sigalarm,
[SYS_sigreturn]  sys_sigreturn,
};

```

```c
//syscall.h
// System call numbers

#define SYS_sigalarm  22
#define SYS_sigreturn  23

```

```c
//usys.pl
#!/usr/bin/perl -w

# Generate usys.S, the stubs for syscalls.

print "# generated by usys.pl - do not edit\n";

print "#include \"kernel/syscall.h\"\n";

sub entry {
    my $name = shift;
    print ".global $name\n";
    print "${name}:\n";
    print " li a7, SYS_${name}\n";
    print " ecall\n";
    print " ret\n";
}
	

entry("sigalarm");
entry("sigreturn");

```

```c
//sysproc.c
uint64
sys_sigalarm(void)
{
  return 0;
}

uint64
sys_sigreturn(void)
{
  return 0;
}
```

test： (no interrupt)

```bash
$ alarmtest
test0 start
..............................................QEMU: Terminated
```
- test fail. don't know why the ticks can over 2, cannot find the reason.
- the initialization of `p->ticks_count` is true, but doesn't work.

```c
test0 start
sysproc.c: ticks = 2
...trap.c: test: ticks = 18 interval = 2
.trap.c: test: ticks = 19 interval = 2
.trap.c: test: ticks = 20 interval = 2
.trap.c: test: ticks = 21 interval = 2
.trap.c: test: ticks = 22 interval = 2
.trap.c: test: ticks = 23 interval = 2
..trap.c: test: ticks = 25 interval = 2
.trap.c: test: ticks = 26 interval = 2
.trap.c: test: ticks = 27 interval = 2
```

```c
//trap.c 
//usertrap()
  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2){
    int interval = p->alarm_intvl;
    uint64 fn = p->alarm_hdler;
    // printf("count = %d\n", p->ticks_count);
    // printf("interval = %d\n", p->alarm_intvl);
    if (interval == 0 && fn == 0)
      goto yield;

    int ticks = p->ticks_count;
    printf("trap.c: test: ticks = %d interval = %d\n", ticks, interval);
    if (ticks == interval){
        p->ticks_count = 0;
        p->trapframe->epc = fn;
      }
    } else {
      p->ticks_count += 1;
    }
```

problem found:

right curly brackets `}` missed.
correct:

```c
  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2){
    int interval = p->alarm_intvl;
    uint64 fn = p->alarm_hdler;
    // printf("count = %d\n", p->ticks_passed);
    // printf("interval = %d\n", p->alarm_intvl);
    if (interval == 0 && fn == 0)
      goto yield;

    int ticks = p->ticks_passed;
    
    if (ticks == interval){
        p->ticks_passed = 0;
        p->trapframe->epc = fn;
      }
    else {
      p->ticks_passed += 1;
    }
  
  yield:
    yield();
  }
  usertrapret();
```


test1/2:  【36】 = 288 / 8 bytes

```c
kernel/proc.h
  int alarm_hdler_called;      // Flag to record whether handler has been called before alarm
  uint64 alarm_saved_tf[36];   // trapframe for alarm
```

```c
//show for reference
//proc.h
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};
```
result:
```bash
$ alarmtest
test0 start
......alarm!
test0 passed
test1 start
.alarm!
.alarm!
.alarm!
.alarm!
alarm!
.alarm!
.alarm!
.alarm!
.alarm!
.alarm!
test1 passed
test2 start
.......alarm!
test2 passed
```

```c
//usertests
.
.
.
.

test preempt: kill... wait... OK
test exitwait: OK
test rmdot: OK
test fourteen: OK
test bigfile: OK
test dirfile: OK
test iref: OK
test forktest: OK
test bigdir: OK
ALL TESTS PASSED
```
