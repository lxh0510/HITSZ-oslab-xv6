#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
find(char *path,char *file)                        //find函数，参数分别为路径和待查找的字符串
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      close(fd);
      return;
    }
  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0){
      printf("ls: cannot stat %s\n", buf);
      continue;
      }
    if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))               //不要递归进入.和..
      continue;
    if (st.type == T_DIR)
      {
        find(buf, file);                                          // 如果是目录类型，则循环执行查找操作
      }
    else if (st.type == T_FILE && !strcmp(de.name, file))           // 如果是文件类型 并且名称与要查找的文件名相同
      {
        printf("%s\n", buf);                                      // 打印路径
      } 
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc != 3)                                   // 如果参数个数不为 3 则报错
    {
        printf("Program Wrong!");
        exit(1);
    }
    find(argv[1], argv[2]);                          // 调用 find 函数进行查找
    exit(0);
}
