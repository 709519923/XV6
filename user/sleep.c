#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;
  if(argc < 2){
    fprintf(2, "error! no parameter.\n");
    exit(1);
  }else{
    i = 1;
    int time  = atoi(argv[i]);
    sleep(time);
    exit(0);
  }
}
