#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXN 1024

int main(int argc, char *argv[]){
    char* argvs[MAXARG];                         //存放子进程的参数列表
    char buf[MAXN] = {"\0"};
    int i;
    if (argc < 2 )                                   // 如果参数个数不为 3 则报错
    {
         printf("Program Wrong!");
         exit(1);
    }

    for(i=1;i<argc;i++){                   //保存命令行参数，略去第一个xargs
        argvs[i-1]=argv[i];
    }

    while(read(0,buf,MAXN)>0)             //从管道循环读取缓冲区数据
    {
        char temp[MAXN]={"\0"};
        argvs[argc-1]=temp;              //追加xargs指令后参数
        for(i=0;i<strlen(buf);i++){         //循环读取追加的参数
            if(buf[i]=='\n')             //读取缓冲区，当读到换行符时，创建子进程
            {
                if(fork()==0){
                    exec(argv[1],argvs);    //子进程执行命令，argv[1]为指令内容，argvs为参数
                }
                wait(0);                //等待子进程结束
            }
            else{
                    temp[i]=buf[i];           //否则将管道输出作为输入
                }
            }
    }

    exit(0);

}