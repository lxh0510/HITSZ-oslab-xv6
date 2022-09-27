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
find(char *path,char *file)
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
      if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))             //不能递归.和..
            continue;
    }
    if (st.type == T_DIR)                                           //如果为目录类型，则递归函数
        {
            find(buf, file);
        }
        else if (st.type == T_FILE && !strcmp(de.name, file))         //如果为文件类型，且名字与待识别字符串相同，则输出路径
        {
            printf("%s\n", buf);
        } 
  close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc != 3)                             // 如果参数个数不为 3 则报错
    {
        printf("Program Wrong\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}