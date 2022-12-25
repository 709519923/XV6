#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;


  return p;
}

void
find(char *path, char *filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;   //These functions return information about a file, in the buffer pointed to by statbuf.

  if((fd = open(path, 0)) < 0){   // 1. open file to path
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){     // 2. get file information to st
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  //about fd, using int because every fd has individual id

  switch(st.type){
  case T_FILE:
    //printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){   //4. while-loop for print
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      if(!strcmp(fmtname(buf), filename)){
        printf("%s\n", buf);
      }
      if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
			  continue;
      //dive into deeper directories
      find(buf, filename);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  //int i;

  // if(argc < 2){
  //   find(".");
  //   exit(0);
  // }
  // for(i=1; i<argc; i++)
  //   find(argv[i]);
  find(argv[1], argv[2]);
  exit(0);
}
