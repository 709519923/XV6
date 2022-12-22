#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int generate_natural(int initial_num, int max_num);
int primes_filter(int fd, int primes);


int
main(int argc, char *argv[])
{
  int first_prime;
  int input = generate_natural(2, 36);
  while(read(input, &first_prime, sizeof(int))){  //read function will return 0 when coming to end
    fprintf(2, "prime %d\n", first_prime);
    input = primes_filter(input, first_prime);
  }
  exit(0);
}

//return a file descriptor for read
int generate_natural(int initial_num, int max_num){
    int p[2];
    pipe(p);
    for(int i = initial_num; i < max_num; i++){
        write(p[1], &i, sizeof(int));
    }
    close(p[1]);
    return p[0];
}

//return a file descriptor for read
int primes_filter(int fd, int primes){
    int readBuf;
    int p[2];
    pipe(p);
    
    //update input  
    while(read(fd, &readBuf, sizeof(int))){  //read function will return 0 when coming to end
        if(readBuf % primes){
            write(p[1], &readBuf, sizeof(int));
        }        
    }
    close(p[1]);
    return p[0];
}