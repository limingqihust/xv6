#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc,char** argv)
{
    // fd[0] is for read,fd[1] is for write
    int ptoc_fd[2],ctop_fd[2];
    char buf[100];
    pipe(ptoc_fd);
    pipe(ctop_fd);
    int pid=fork();
    if(pid!=0)              // parent process
    {
        write(ptoc_fd[1],"ping",4);
        read(ctop_fd[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
    }
    else                    // child process
    {
        read(ptoc_fd[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
        write(ctop_fd[1],"pong",4);
    }
    exit(0);
}