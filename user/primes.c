#include "kernel/types.h"
#include "user.h"
#include "user/user.h"
#include "stddef.h"

void
mapping(int n, int pd[])                // 将管道的 读或写 端口映射到描述符 n 上
{
  close(n);                           
  dup(pd[n]);
  close(pd[0]);
  close(pd[1]);
}

void
primes()
{
  int current, next;
  int pd[2];
  if (read(0, &current, sizeof(int)))
  {
    printf("prime %d\n", current);            // 第一个一定是素数
    pipe(pd);
    if (fork() == 0)                          //子进程
    {
      mapping(0, pd);                         // 将管道的读端口映射到描述符 0 上
      primes();                               //递归寻找素数
    }
    else                                      // 父进程
    {
      mapping(1, pd);                         // 将管道的写端口映射到描述符 1 上
      while (read(0, &next, sizeof(int)))
      {
        if (next % current != 0)
        {
          write(1, &next, sizeof(int));
        }
      }
      wait(0);                            //等待子进程结束
      exit(0);  
    }  
  }
  exit(0);  
}

int 
main(int argc, char *argv[])
{
  int pd[2];
  pipe(pd);
  if (fork() == 0)                       //子进程
  {
    mapping(0, pd);               // 将管道的读端口映射到描述符 0 上
    primes();
  }
  else                           // 父进程
  {
    mapping(1, pd);                  // 将管道的写端口映射到描述符 1 上
    for (int i = 2; i < 36; i++)     // 循环获取 2 至 35
    {
      write(1, &i, sizeof(int));
    }
    wait(0);                  //等待子进程结束
    exit(0);
  }
    exit(0);
}
