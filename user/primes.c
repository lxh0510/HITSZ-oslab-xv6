#include "kernel/types.h"
#include "user.h"
#include <stddef.h>

void mapping(int n,int pd[]){           //重定向文件描述符
    close(n);
    dup(pd[n]);
    close(pd[0]);
    close(pd[1]);
}

void primes()
{
    int current,next;
    int fd[2];
    pipe(fd);
    if(read(0,&current,sizeof(int))){      //读取第一个数据，该数据一定为质数
        printf("prime %d\n",current);
        int pid=fork();
        if(pid==0)                        //子进程
        {
            mapping(1,fd);                // 子进程将管道的写端口映射到描述符1上
            while(read(0,&next,sizeof(int)))
            {
                if(next%current!=0)              //如果该数据不能整除第一个质数
                { 
                    write(1,&next,sizeof(int));       //将该数据写入管道
                }
            }
        }
        else{
            wait(NULL);                          //等待子进程结束
            mapping(0,fd);                    //父进程将管道的读端口映射到描述符0上
            primes();                         //循环执行该函数
        }
    }
}

int main(int argc,char* argv[]){
    int pd[2];
    pipe(pd);
    int pid=fork();
    if(pid==0){
        mapping(1,pd);
        for(int i=2;i<=35;i++){              //子进程将2-35写入管道
            write(1,&i,sizeof(int));
        }
    }
    else{
        wait(NULL);                          //等待子进程结束
        mapping(0,pd);                    //父进程将管道的读端口映射到描述符0上
        primes();                         //对子进程写入数据执行素数筛选函数
    
    }
    exit(0);
}