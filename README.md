# File System

**inode**: a file’s size and list of data block numbers.

```c
// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};
```

## Large files

not difficult, just use a double index to enlarge the file sizes.

```diff
diff --git a/kernel/file.h b/kernel/file.h
index b076d1d..5c4eb3a 100644
--- a/kernel/file.h
+++ b/kernel/file.h
@@ -26,7 +26,7 @@ struct inode {
   short minor;
   short nlink;
   uint size;
-  uint addrs[NDIRECT+1];
+  uint addrs[NDIRECT+2];
 };
 
 // map major device number to device functions.
diff --git a/kernel/fs.c b/kernel/fs.c
index 40c9bd4..842617e 100644
--- a/kernel/fs.c
+++ b/kernel/fs.c
@@ -401,6 +401,35 @@ bmap(struct inode *ip, uint bn)
     return addr;
   }
 
+  bn -= NINDIRECT;
+
+  if(bn < NDINDIRECT){
+    // Load load double-indirect block, allocating if necessary.
+    int level2_idx = bn / NADDR_PER_BLOCK;  // 要查找的块号位于二级间接块中的位置
+    int level1_idx = bn % NADDR_PER_BLOCK;  // 要查找的块号位于一级间接块中的位置
+    if((addr = ip->addrs[NDIRECT+1]) == 0)
+      ip->addrs[NDIRECT+1] = addr = balloc(ip->dev);
+    bp = bread(ip->dev, addr);
+    a = (uint*)bp->data;
+
+    if((addr = a[level2_idx]) == 0){
+      a[level2_idx] = addr = balloc(ip->dev);
+      log_write(bp);
+    }
+    brelse(bp);
+
+    bp = bread(ip->dev, addr);
+    a = (uint*)bp->data;
+
+    if((addr = a[level1_idx]) == 0){
+      a[level1_idx] = addr = balloc(ip->dev);
+      log_write(bp);
+    }
+    brelse(bp);
+
+    return addr;
+  }  
+
   panic("bmap: out of range");
 }
 
@@ -432,6 +461,30 @@ itrunc(struct inode *ip)
     ip->addrs[NDIRECT] = 0;
   }
 
+  struct buf* bp1;
+  uint* a1;
+  if(ip->addrs[NDIRECT + 1]) {
+    bp = bread(ip->dev, ip->addrs[NDIRECT + 1]);
+    a = (uint*)bp->data;
+    for(i = 0; i < NADDR_PER_BLOCK; i++) {
+      // 每个一级间接块的操作都类似于上面的
+      // if(ip->addrs[NDIRECT])中的内容
+      if(a[i]) {
+        bp1 = bread(ip->dev, a[i]);
+        a1 = (uint*)bp1->data;
+        for(j = 0; j < NADDR_PER_BLOCK; j++) {
+          if(a1[j])
+            bfree(ip->dev, a1[j]);
+        }
+        brelse(bp1);
+        bfree(ip->dev, a[i]);
+      }
+    }
+    brelse(bp);
+    bfree(ip->dev, ip->addrs[NDIRECT + 1]);
+    ip->addrs[NDIRECT + 1] = 0;
+  }
+
   ip->size = 0;
   iupdate(ip);
 }
diff --git a/kernel/fs.h b/kernel/fs.h
index 139dcc9..eb08e4a 100644
--- a/kernel/fs.h
+++ b/kernel/fs.h
@@ -5,6 +5,7 @@
 #define ROOTINO  1   // root i-number
 #define BSIZE 1024  // block size
 
+
 // Disk layout:
 // [ boot block | super block | log | inode blocks |
 //                                          free bit map | data b
locks]
@@ -24,9 +25,13 @@ struct superblock {
 
 #define FSMAGIC 0x10203040
 
-#define NDIRECT 12
+
+
+#define NDIRECT 11
 #define NINDIRECT (BSIZE / sizeof(uint))
-#define MAXFILE (NDIRECT + NINDIRECT)
+#define NDINDIRECT ((BSIZE / sizeof(uint)) * (BSIZE / sizeof(uint
)))
+#define MAXFILE (NDIRECT + NINDIRECT + NDINDIRECT)
+#define NADDR_PER_BLOCK (BSIZE / sizeof(uint))  // 一个块中的地址
数量
 
 // On-disk inode structure
 struct dinode {
@@ -35,7 +40,7 @@ struct dinode {
   short minor;          // Minor device number (T_DEVICE only)
   short nlink;          // Number of links to inode in file syste
m
   uint size;            // Size of file (bytes)
-  uint addrs[NDIRECT+1];   // Data block addresses
+  uint addrs[NDIRECT+2];   // Data block addresses
 };
 
 // Inodes per block.
```



## Symbolic links (short cut)

1. add some definition:

```c
// fcntl.h
#define O_NOFOLLOW 0x004
// stat.h
#define T_SYMLINK 4
```

2. write `target path` to a new `inode`

```c
uint64
sys_symlink(void) {
  char target[MAXPATH], path[MAXPATH];
  struct inode* ip_path;

  if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0) {
    return -1;
  }

  begin_op();
  // 分配一个inode结点，create返回锁定的inode
  ip_path = create(path, T_SYMLINK, 0, 0);
  if(ip_path == 0) {
    end_op();
    return -1;
  }
  // 向inode数据块中写入target路径
  if(writei(ip_path, 0, (uint64)target, 0, MAXPATH) < MAXPATH) {
    iunlockput(ip_path);
    end_op();
    return -1;
  }

  iunlockput(ip_path);
  end_op();
  return 0;
}

```

3. change `sys_open` to support symbolic link.

```c
uint64
sys_open(void)
{
  ...

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    ...
  }

  // 处理符号链接
  if(ip->type == T_SYMLINK && !(omode & O_NOFOLLOW)) {
    // 若符号链接指向的仍然是符号链接，则递归的跟随它
    // 直到找到真正指向的文件
    // 但深度不能超过MAX_SYMLINK_DEPTH
    for(int i = 0; i < MAX_SYMLINK_DEPTH; ++i) {
      // 读出符号链接指向的路径
      if(readi(ip, 0, (uint64)path, 0, MAXPATH) != MAXPATH) {
        iunlockput(ip);
        end_op();
        return -1;
      }
      iunlockput(ip);
      ip = namei(path);
      if(ip == 0) {
        end_op();
        return -1;
      }
      ilock(ip);
      if(ip->type != T_SYMLINK)
        break;
    }
    // 超过最大允许深度后仍然为符号链接，则返回错误
    if(ip->type == T_SYMLINK) {
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    ...
  }

  ...
  return fd;
}

```

**log**:

```diff
diff --git a/kernel/fs.h b/kernel/fs.h
index 139dcc9..eb08e4a 100644
--- a/kernel/fs.h
+++ b/kernel/fs.h
@@ -5,6 +5,7 @@
 #define ROOTINO  1   // root i-number
 #define BSIZE 1024  // block size
 
+
 // Disk layout:
 // [ boot block | super block | log | inode blocks |
 //                                          free bit map | data blocks]
@@ -24,9 +25,13 @@ struct superblock {
 
 #define FSMAGIC 0x10203040
 
-#define NDIRECT 12
+
+
+#define NDIRECT 11
 #define NINDIRECT (BSIZE / sizeof(uint))
-#define MAXFILE (NDIRECT + NINDIRECT)
+#define NDINDIRECT ((BSIZE / sizeof(uint)) * (BSIZE / sizeof(uint)))
+#define MAXFILE (NDIRECT + NINDIRECT + NDINDIRECT)
+#define NADDR_PER_BLOCK (BSIZE / sizeof(uint))  // 一个块中的地址数量
 
 // On-disk inode structure
 struct dinode {
@@ -35,7 +40,7 @@ struct dinode {
   short minor;          // Minor device number (T_DEVICE only)
   short nlink;          // Number of links to inode in file system
   uint size;            // Size of file (bytes)
-  uint addrs[NDIRECT+1];   // Data block addresses
+  uint addrs[NDIRECT+2];   // Data block addresses
 };
 
 // Inodes per block.
diff --git a/kernel/stat.h b/kernel/stat.h
index 19543af..6dc12d2 100644
--- a/kernel/stat.h
+++ b/kernel/stat.h
@@ -1,6 +1,8 @@
 #define T_DIR     1   // Directory
 #define T_FILE    2   // File
 #define T_DEVICE  3   // Device
+#define T_SYMLINK 4   // link
+
 
 struct stat {
   int dev;     // File system's disk device
diff --git a/kernel/syscall.c b/kernel/syscall.c
index c1b3670..1697b62 100644
--- a/kernel/syscall.c
+++ b/kernel/syscall.c
@@ -104,6 +104,7 @@ extern uint64 sys_unlink(void);
 extern uint64 sys_wait(void);
 extern uint64 sys_write(void);
 extern uint64 sys_uptime(void);
+extern uint64 sys_symlink(void);
 
 static uint64 (*syscalls[])(void) = {
 [SYS_fork]    sys_fork,
@@ -127,6 +128,7 @@ static uint64 (*syscalls[])(void) = {
 [SYS_link]    sys_link,
 [SYS_mkdir]   sys_mkdir,
 [SYS_close]   sys_close,
+[SYS_symlink] sys_symlink,
 };
 
 void
diff --git a/kernel/syscall.h b/kernel/syscall.h
index bc5f356..97e4b87 100644
--- a/kernel/syscall.h
+++ b/kernel/syscall.h
@@ -20,3 +20,4 @@
 #define SYS_link   19
 #define SYS_mkdir  20
 #define SYS_close  21
+#define SYS_symlink  22
diff --git a/kernel/sysfile.c b/kernel/sysfile.c
index 5dc453b..af5bc4a 100644
--- a/kernel/sysfile.c
+++ b/kernel/sysfile.c
@@ -3,7 +3,7 @@
 // Mostly argument checking, since we don't trust
 // user code, and calls into file.c and fs.c.
 //
-
+#define MAX_SYMLINK_DEPTH 10
 #include "types.h"
 #include "riscv.h"
 #include "defs.h"
@@ -322,6 +322,36 @@ sys_open(void)
     return -1;
   }
 
+  // 处理符号链接
+  if(ip->type == T_SYMLINK && !(omode & O_NOFOLLOW)) {
+    // 若符号链接指向的仍然是符号链接，则递归的跟随它
+    // 直到找到真正指向的文件
+    // 但深度不能超过MAX_SYMLINK_DEPTH
+    for(int i = 0; i < MAX_SYMLINK_DEPTH; ++i) {
+      // 读出符号链接指向的路径
+      if(readi(ip, 0, (uint64)path, 0, MAXPATH) != MAXPATH) {
+        iunlockput(ip);
+        end_op();
+        return -1;
+      }
+      iunlockput(ip);
+      ip = namei(path);
+      if(ip == 0) {
+        end_op();
+        return -1;
+      }
+      ilock(ip);
+      if(ip->type != T_SYMLINK)
+        break;
+    }
+    // 超过最大允许深度后仍然为符号链接，则返回错误
+    if(ip->type == T_SYMLINK) {
+      iunlockput(ip);
+      end_op();
+      return -1;
+    }
+  }
+
   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
     if(f)
       fileclose(f);
@@ -484,3 +514,31 @@ sys_pipe(void)
   }
   return 0;
 }
+
+uint64
+sys_symlink(void) {
+  char target[MAXPATH], path[MAXPATH];
+  struct inode* ip_path;
+
+  if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0) {
+    return -1;
+  }
+
+  begin_op();
+  // 分配一个inode结点，create返回锁定的inode
+  ip_path = create(path, T_SYMLINK, 0, 0);
+  if(ip_path == 0) {
+    end_op();
+    return -1;
+  }
+  // 向inode数据块中写入target路径
+  if(writei(ip_path, 0, (uint64)target, 0, MAXPATH) < MAXPATH) {
+    iunlockput(ip_path);
+    end_op();
+    return -1;
+  }
+
+  iunlockput(ip_path);
+  end_op();
+  return 0;
+}
diff --git a/user/user.h b/user/user.h
index b71ecda..446b52a 100644
--- a/user/user.h
+++ b/user/user.h
@@ -23,6 +23,7 @@ int getpid(void);
 char* sbrk(int);
 int sleep(int);
 int uptime(void);
+int symlink(char *target, char *path);
 
 // ulib.c
 int stat(const char*, struct stat*);
diff --git a/user/usys.pl b/user/usys.pl
index 01e426e..bc5c22e 100755
--- a/user/usys.pl
+++ b/user/usys.pl
@@ -36,3 +36,4 @@ entry("getpid");
 entry("sbrk");
 entry("sleep");
 entry("uptime");
+entry("symlink");
```



**result:**

```bash
$ symlinktest
Start: test symlinks
test symlinks: ok
Start: test concurrent symlinks
test concurrent symlinks: ok
```

