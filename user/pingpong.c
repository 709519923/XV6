#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p[2];
  char* cbuf[6];
  pipe(p);
  if(fork() == 0){ //child
    //step2
    read(p[0], cbuf, 6);
    fprintf(2,"%d: received %s", getpid(), cbuf);
    //close(p[0]);
    
    //step3
    write(p[1], "pong\n", 6);
    exit(0);
  }else{  //parents
    //step1

    write(p[1], "ping\n", 6);

    wait(0);
    
    //step4

    read(p[0], cbuf, 6);
    fprintf(2,"%d: received %s", getpid(), cbuf);

  }
  exit(0);
}
