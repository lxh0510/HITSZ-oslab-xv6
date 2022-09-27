#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXN 1024

int main(int argc, char *argv[]){
    char* argvs[MAXARG];                         //存放子进程的参数列表
    char buf[MAXN] = {"\0"};
    int i;
    // if (argc < 2 )                                   // 如果参数个数不为 3 则报错
    // {
    //     printf("Program Wrong!");
    //     exit(1);
    // }

    for(i=1;i<argc;i++){                   //保存命令行参数，略去第一个xargs
        argvs[i-1]=argv[i];
    }

    int n;
    while((n = read(0,buf,MAXN))>0)
    {
        char temp[MAXN]={"\0"};
        argvs[argc-1]=temp;
        for(i=0;i<strlen(buf);i++){
            if(buf[i]=='\n')
            {
                if(fork()==0){
                    exec(argv[1],argvs);
                }
                wait(0);
            }
            else{
                    temp[i]=buf[i];
                }
            }
    }

    exit(0);

}