#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"



int mystrstr(char* path,char* filename)
{
    int len_path=strlen(path);
    int i;
    for(i=len_path;;i--)
    {
        if(path[i]=='/')
            break;
    }
    if(!strcmp(path+i+1,filename))
        return 1;
    return 0;
}


// ��path�в���filename
// ���path��Ŀ¼�ļ� ���������ļ��в���filename
// ���path�������ļ� ƥ��path��filename���ļ���
void find(char *path, char* filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type)
  {
    case T_DEVICE:

    // �����ļ�
    case T_FILE:
        // printf("%s\t%d\t%d\t%l\n", path, st.type, st.ino, st.size);
        if(mystrstr(path,filename))
            printf("%s\n",path);
        break;
        
    // Ŀ¼�ļ�
    case T_DIR:
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if(de.inum == 0)
                continue;
            char str0[10]=".";
            char str1[10]="..";
            if(!strcmp(de.name,str0) || !strcmp(de.name,str1))
                continue;
            memmove(p, de.name, DIRSIZ);
            
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            // printf("%s\t%d\t%d\t%d\n", buf, st.type, st.ino, st.size);
            find(buf,filename);
        }
        break;
  }
  close(fd);
}


int main(int argc,char* argv[])
{
    if(argc<3)
    {
        fprintf(2,"Usage: find [path] [filename]\n");
        exit(-1);
    }
    find(argv[1],argv[2]);
    exit(0);
}