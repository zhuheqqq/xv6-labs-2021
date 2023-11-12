#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void foo(int *left_fd)
{
    close(left_fd[1]);
    int prime,i;
    read(left_fd[0],&prime,sizeof(int));//第一个数字是素数
    printf("prime %d\n",prime);

    if(read(left_fd[0],&i,sizeof(int))!=0)
    {
        int right_fd[2];
        pipe(right_fd);
        int pid=fork();
        if(pid==0)
        {
            foo(right_fd);
        }else if(pid>0)
        {
            close(right_fd[0]);
            if(i%prime!=0)
            {
                write(right_fd[1],&i,sizeof(int));
            }
            while(read(left_fd[0],&i,sizeof(int))!=0)
            {
                if(i%prime!=0)
                {
                    write(right_fd[1],&i,sizeof(int));
                }
            }

            close(right_fd[1]);
            close(left_fd[0]);
            wait(0);//父进程等待子进程
        }
    }
}

int main(int argc,char* argv[])
{
    int left_fd[2];
    pipe(left_fd);

    int pid=fork();

    if(pid==0)
    {
        foo(left_fd);
    }else if(pid>0)
    {
        close(left_fd[0]);
        int i;
        for(i=2;i<=35;i++)
        {
            write(left_fd[1],&i,sizeof(int));
        }

        close(left_fd[1]);
        wait(0);
    }

    exit(0);
}
