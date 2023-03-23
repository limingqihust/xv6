#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAXN 35
#define WRITE 1
#define READ 0

// 父进程传向子进程
// 接受数据 第一个数一定为质数 对剩下的数做筛选 并传递给子进程
void primes(int rd)
{
    int prime;
    if(read(rd,&prime,sizeof(int))==0) return;
    printf("prime %d\n",prime);

    int fd[2];
    pipe(fd);
    // printf("the pipe is:READ %d WRITE %d\n",fd[0],fd[1]);
    int pid=fork();
    if(pid==0)          // child process
    {
        close(fd[WRITE]);
        primes(fd[READ]);
        close(fd[READ]);
    }
    else                // parent process
    {
        close(fd[READ]);
        int num[MAXN];
        for(int i=0;;i++)
        {
            int len=read(rd,&num[i],sizeof(int));
            if(len==0) break;
            if(num[i]%prime)
            {
                write(fd[WRITE],&num[i],sizeof(int));
                // printf("write %d to pipe %d\n",num[i],fd[WRITE]);
            }
        }
        close(fd[WRITE]);
        wait(&pid);
    }
    
}


int main(int argc,char** argv)
{
    int fd[2];
    pipe(fd);       // fd[0] for read,fd[1] for write
    // printf("the pipe is:READ %d WRITE %d\n",fd[0],fd[1]);
    int pid=fork();
    if(pid==0)      // child process
    {
        close(fd[WRITE]);
        primes(fd[READ]);
        close(fd[READ]);
    }
    else            // parent process
    {
        close(fd[READ]);
        for(int i=2;i<=MAXN;i++)
        {
            write(fd[WRITE],&i,sizeof(int));
        }
        close(fd[WRITE]);
        wait(&pid);
    }
    exit(0);
}