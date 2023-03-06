# Multithread

for reference:

[http://xv6.dgs.zone/labs/answers/lab7.html]: http://xv6.dgs.zone/labs/answers/lab7.html

## Uthread: switching between threads

1. copy the kernel thread switch to user thread   (configuration for thread schedule)

```c
//uthread_switch.S
.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd s0, 16(a0)
	sd s1, 24(a0)
	sd s2, 32(a0)
	sd s3, 40(a0)
	sd s4, 48(a0)
	sd s5, 56(a0)
	sd s6, 64(a0)
	sd s7, 72(a0)
	sd s8, 80(a0)
	sd s9, 88(a0)
	sd s10, 96(a0)
	sd s11, 104(a0)

	ld ra, 0(a1)
	ld sp, 8(a1)
	ld s0, 16(a1)
	ld s1, 24(a1)
	ld s2, 32(a1)
	ld s3, 40(a1)
	ld s4, 48(a1)
	ld s5, 56(a1)
	ld s6, 64(a1)
	ld s7, 72(a1)
	ld s8, 80(a1)
	ld s9, 88(a1)
	ld s10, 96(a1)
	ld s11, 104(a1)
	ret    /* return to ra */

```

2. content switch

   ```c
   //uthread.c
   void 
   thread_schedule(void)
   {
     struct thread *t, *next_thread;
   
     /* Find another runnable thread. */
     next_thread = 0;
     t = current_thread + 1;
     for(int i = 0; i < MAX_THREAD; i++){
       if(t >= all_thread + MAX_THREAD)
         t = all_thread;
       if(t->state == RUNNABLE) {
         next_thread = t;
         break;
       }
       t = t + 1;
     }
   
     if (next_thread == 0) {
       printf("thread_schedule: no runnable threads\n");
       exit(-1);
     }
   
     if (current_thread != next_thread) {         /* switch threads?  */
       next_thread->state = RUNNING;
       t = current_thread;
       current_thread = next_thread;
       /* YOUR CODE HERE
        * Invoke thread_switch to switch from t to next_thread:
        * thread_switch(??, ??);
        */
       thread_switch((uint64)&t->context, (uint64)&current_thread->context);
     } else
       next_thread = 0;
   }		
   ```

   ```c
   void 
   thread_create(void (*func)())
   {
     struct thread *t;
   
     for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
       if (t->state == FREE) break;
     }
     t->state = RUNNABLE;
     // YOUR CODE HERE
     t->context.ra = (uint64)func;                   // 设定函数返回地址
     t->context.sp = (uint64)t->stack + STACK_SIZE;  // 设定栈指针
   
   }
   
   ```

   

## Using threads

add lock to the 5 buckets using `pthread` API.

```diff
diff --git a/notxv6/ph.c b/notxv6/ph.c
index 82afe76..5ae4108 100644
--- a/notxv6/ph.c
+++ b/notxv6/ph.c
@@ -7,6 +7,14 @@
 
 #define NBUCKET 5
 #define NKEYS 100000
+pthread_mutex_t locks[NBUCKET];
+
+void
+init_locks()
+{
+  for (int i = 0; i < NBUCKET; i++)
+    assert(pthread_mutex_init(&locks[i], NULL) == 0);
+}
 
 struct entry {
   int key;
@@ -52,7 +60,9 @@ void put(int key, int value)
     e->value = value;
   } else {
     // the new is new.
+    pthread_mutex_lock(&locks[i]);       // acquire lock
     insert(key, value, &table[i], table[i]);
+    pthread_mutex_unlock(&locks[i]);     // release lock
   }
 
 }
@@ -117,7 +127,7 @@ main(int argc, char *argv[])
   for (int i = 0; i < NKEYS; i++) {
     keys[i] = random();
   }
-
+  init_locks();
   //
   // first the puts
   //
```

## Barrier

1. when a thread check the **barrier**, it need to stop other running threads visit **barrier**, that is to say, it need a lock for the barrier.

2. if condition not fulfil, put thread to the **waiting area**. if yes, release all thread from the **waiting area**   

   

   ```c
   struct barrier {
     pthread_mutex_t barrier_mutex; // lock to stop other threads visit barrier
     pthread_cond_t barrier_cond; // waiting area
     int nthread;      // Number of threads that have reached this round of the barrier
     int round;     // Barrier round
   } bstate;
   ```

   ```diff
   diff --git a/notxv6/barrier.c b/notxv6/barrier.c
   index 12793e8..51f8cc5 100644
   --- a/notxv6/barrier.c
   +++ b/notxv6/barrier.c
   @@ -30,7 +30,18 @@ barrier()
      // Block until all threads have called barrier() and
      // then increment bstate.round.
      //
   -  
   +
   +  //
   +  pthread_mutex_lock(&bstate.barrier_mutex); // apply for thead state lock
   +  bstate.nthread++;
   +  if(bstate.nthread == nthread){
   +    bstate.nthread = 0;
   +    bstate.round++;
   +    pthread_cond_broadcast(&bstate.barrier_cond); 
   +  } else{
   +    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
   +  }
   +  pthread_mutex_unlock(&bstate.barrier_mutex); // apply for thead state lock
    }
   ```

   
