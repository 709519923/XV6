# Networking

**Target**: 

To implement a simple network driver by reading the E1000 dev. manual. 

**method**:

use a ring to check whether the buffer is full or not for TX and RX.

check the register to get information.

**reference**:

[[mit6.s081\] Lab11: Networking(juejin.cn)](https://juejin.cn/post/7022441307029110821)



16 descriptors (receiving ring) for writing directly from network to RAM by DMA mechanism.

```c
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));
```

register address (example):

```c
/* Registers */
#define E1000_CTL      (0x00000/4)  /* Device Control Register - RW */
#define E1000_ICR      (0x000C0/4)  /* Interrupt Cause Read - R */
#define E1000_IMS      (0x000D0/4)  /* Interrupt Mask Set - RW */
#define E1000_RCTL     (0x00100/4)  /* RX Control - RW */
#define E1000_TCTL     (0x00400/4)  /* TX Control - RW */
#define E1000_TIPG     (0x00410/4)  /* TX Inter-packet gap -RW */
#define E1000_RDBAL    (0x02800/4)  /* RX Descriptor Base Address Low - RW */
#define E1000_RDTR     (0x02820/4)  /* RX Delay Timer */
#define E1000_RADV     (0x0282C/4)  /* RX Interrupt Absolute Delay Timer */
#define E1000_RDH      (0x02810/4)  /* RX Descriptor Head - RW */
#define E1000_RDT      (0x02818/4)  /* RX Descriptor Tail - RW */
#define E1000_RDLEN    (0x02808/4)  /* RX Descriptor Length - RW */
#define E1000_RSRPD    (0x02C00/4)  /* RX Small Packet Detect Interrupt */
#define E1000_TDBAL    (0x03800/4)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDLEN    (0x03808/4)  /* TX Descriptor Length - RW */
#define E1000_TDH      (0x03810/4)  /* TX Descriptor Head - RW */
#define E1000_TDT      (0x03818/4)  /* TX Descripotr Tail - RW */
#define E1000_MTA      (0x05200/4)  /* Multicast Table Array - RW Array */
#define E1000_RA       (0x05400/4)  /* Receive Address - RW Array */
```

message buffer:

```c
struct mbuf {
  struct mbuf  *next; // the next mbuf in the chain
  char         *head; // the current start position of the buffer
  unsigned int len;   // the length of the buffer
  char         buf[MBUF_SIZE]; // the backing store
};

```



here is the code added:

```c
diff --git a/kernel/e1000.c b/kernel/e1000.c
index 70a2adf..91b4eaa 100644
--- a/kernel/e1000.c
+++ b/kernel/e1000.c
@@ -102,7 +102,36 @@ e1000_transmit(struct mbuf *m)
   // the TX descriptor ring so that the e1000 sends it. Stash
   // a pointer so that it can be freed after sending.
   //
+  acquire(&e1000_lock); // 获取 E1000 的锁，防止多进程同时发送数据出现 race
+
+  uint32 ind = regs[E1000_TDT]; // 下一个可用的 buffer 的下标
+  struct tx_desc *desc = &tx_ring[ind]; // 获取 buffer 的描述符，其中存储了关于该 buffer 的各种信息
+  // 如果该 buffer 中的数据还未传输完，则代表我们已经将环形 buffer 列表全部用完，缓冲区不足，返回错误
+  if(!(desc->status & E1000_TXD_STAT_DD)) {
+    release(&e1000_lock);
+    return -1;
+  }
+  
+  // 如果该下标仍有之前发送完毕但未释放的 mbuf，则释放
+  if(tx_mbufs[ind]) {
+    mbuffree(tx_mbufs[ind]);
+    tx_mbufs[ind] = 0;
+  }
+
+  // 将要发送的 mbuf 的内存地址与长度填写到发送描述符中
+  desc->addr = (uint64)m->head;
+  desc->length = m->len;
+  // 设置参数，EOP 表示该 buffer 含有一个完整的 packet
+  // RS 告诉网卡在发送完成后，设置 status 中的 E1000_TXD_STAT_DD 位，表示发送完成。
+  desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
+  // 保留新 mbuf 的指针，方便后续再次用到同一下标时释放。
+  tx_mbufs[ind] = m;
+
+  // 环形缓冲区内下标增加一。
+  regs[E1000_TDT] = (regs[E1000_TDT] + 1) % TX_RING_SIZE;
   
+  release(&e1000_lock);
+
   return 0;
 }
 
@@ -115,6 +144,27 @@ e1000_recv(void)
   // Check for packets that have arrived from the e1000
   // Create and deliver an mbuf for each packet (using net_rx()).
   //
+  while(1) { // 每次 recv 可能接收多个包
+
+    uint32 ind = (regs[E1000_RDT] + 1) % RX_RING_SIZE;
+    
+    struct rx_desc *desc = &rx_ring[ind];
+    // 如果需要接收的包都已经接收完毕，则退出
+    if(!(desc->status & E1000_RXD_STAT_DD)) {
+      return;
+    }
+
+    rx_mbufs[ind]->len = desc->length;
+    
+    net_rx(rx_mbufs[ind]); // 传递给上层网络栈。上层负责释放 mbuf
+
+    // 分配并设置新的 mbuf，供给下一次轮到该下标时使用
+    rx_mbufs[ind] = mbufalloc(0); 
+    desc->addr = (uint64)rx_mbufs[ind]->head;
+    desc->status = 0;
+
+    regs[E1000_RDT] = ind;
+  }
 }
```

network scheme for deal with message: **Polling Scheme** (轮询模式)

OS just has 1 thread to deal with packet. if many message coming at once and invoke interrupt, it would be waste for CPU to get into trap to deal with them.

to avoid frequent interrupt of high work load, after first interrupt, we set the interrupt-caused register disabled, which is called **Polling Scheme**. reference:[【MIT 6.S081 】lab11 network -  (zhihu.com)](https://zhuanlan.zhihu.com/p/593943193)

```c
void
e1000_intr(void)
{
  // tell the e1000 we've seen this interrupt;
  // without this the e1000 won't raise any
  // further interrupts.
  regs[E1000_ICR] = 0xffffffff; // to let 

  e1000_recv();
}
```





**result:**

```bash
$ make server 
python3 server.py 26099
listening on localhost port 26099
a message from xv6!
a message from xv6!
```

```bash
make qemu
nettests

$ nettests
nettests running on port 26099
testing ping: OK
testing single-process pings: OK
testing multi-process pings: OK
testing DNS
DNS arecord for pdos.csail.mit.edu. is 128.52.129.126
DNS OK
all tests passed.
```

```bash
$ tcpdump -XXnr packets.pcap
...
09:06:30.103948 IP 10.0.2.2.26099 > 10.0.2.15.2002: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 006b 0000 4011 6245 0a00 0202 0a00  .-.k..@.bE......
        0x0020:  020f 65f3 07d2 0019 3214 7468 6973 2069  ..e.....2.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
09:06:30.103955 IP 10.0.2.2.26099 > 10.0.2.15.2001: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 006c 0000 4011 6244 0a00 0202 0a00  .-.l..@.bD......
        0x0020:  020f 65f3 07d1 0019 3215 7468 6973 2069  ..e.....2.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
09:06:30.103962 IP 10.0.2.2.26099 > 10.0.2.15.2008: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 006d 0000 4011 6243 0a00 0202 0a00  .-.m..@.bC......
        0x0020:  020f 65f3 07d8 0019 320e 7468 6973 2069  ..e.....2.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
09:06:30.103970 IP 10.0.2.2.26099 > 10.0.2.15.2007: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 006e 0000 4011 6242 0a00 0202 0a00  .-.n..@.bB......
        0x0020:  020f 65f3 07d7 0019 320f 7468 6973 2069  ..e.....2.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
09:06:30.191444 IP 10.0.2.15.10000 > 8.8.8.8.53: 6828+ A? pdos.csail.mit.edu. (36)
        0x0000:  ffff ffff ffff 5254 0012 3456 0800 4500  ......RT..4V..E.
        0x0010:  0040 0000 0000 6411 3a8f 0a00 020f 0808  .@....d.:.......
        0x0020:  0808 2710 0035 002c 0000 1aac 0100 0001  ..'..5.,........
        0x0030:  0000 0000 0000 0470 646f 7305 6373 6169  .......pdos.csai
        0x0040:  6c03 6d69 7403 6564 7500 0001 0001       l.mit.edu.....
09:06:30.278089 IP 8.8.8.8.53 > 10.0.2.15.10000: 6828 1/0/0 A 128.52.129.126 (52)
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  0050 006f 0000 4011 5e10 0808 0808 0a00  .P.o..@.^.......
        0x0020:  020f 0035 2710 003c 9458 1aac 8180 0001  ...5'..<.X......
        0x0030:  0001 0000 0000 0470 646f 7305 6373 6169  .......pdos.csai
        0x0040:  6c03 6d69 7403 6564 7500 0001 0001 c00c  l.mit.edu.......
        0x0050:  0001 0001 0000 012d 0004 8034 817e       .......-...4.~
```

