#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
    int i;
    int cmd_len = 0;
    int buf_len;
    char buf[32];
    char *cmd[MAXARG];
    //读取 xargs 后面的参数，即最终要执行的命令
    for(i = 1; i < argc; i++){
        cmd[cmd_len++] = argv[i];      
    }
    printf("cmd = %s\n", *(cmd+1));
    //read读取管道的输出，即面前的命令的执行结果，并进行处理
    while((buf_len = read(0, buf, sizeof(buf))) > 0){
        
        char xargs[32] = {"\0"}; //追加参数
        cmd[cmd_len] = xargs;
        for(i = 0; i < buf_len; i++){
            if(buf[i] == '\n'){
                if(fork() == 0){
                    exec(argv[1], cmd);
                }                
                wait(0);
            } else {
                xargs[i] = buf[i];
                printf("%s\n", xargs);
            }
        }
    }
    exit(0);
}

