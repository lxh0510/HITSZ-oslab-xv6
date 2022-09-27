#include "kernel/types.h"
#include "user.h"
int main(int argc,char* argv[]){
    int p1[2],p2[2];
    pipe(p1);
    pipe(p2);
    char buf1[4] ;
    char buf2[4] ;
    if (fork() == 0)
    {
        /* 子进程 */
        close(p1[1]);
        read(p1[0],buf1,4);
        printf("%d: received %s\n",getpid(),buf1);
        close(p1[0]); // 读取完成，关闭读端
        close(p2[0]);
        write(p2[1],"pong",4);
        close(p2[1]); // 写入完成，关闭写端
    }
    else
    {
        /* 父进程 */
        close(p1[0]);
        write(p1[1],"ping",4);
        close(p1[1]); // 写入完成，关闭写端
        close(p2[1]);
        read(p2[0],buf2,4); 
        printf("%d: received %s\n",getpid(),buf2);
        close(p2[0]); // 读入完成，关闭读端
    }
    exit(0);
}