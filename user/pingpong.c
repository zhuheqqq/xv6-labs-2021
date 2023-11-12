/*int fd[2]创建管道，fd[0]读、fd[1]写
管道用法：一般先创建一个管道，然后进程使用fork函数创建子进程，之后父进程关闭管道的读端，子进程关闭管道的写端
调用fork 后，父进程的 fork() 会返回子进程的 PID，子进程的fork返回 0
注意到write系统调用是ssize_t write(int fd, const void *buf, size_t count);，因此写入的字节是一个地址
由于我们声明buf是一个char类型，因此需要填入&buf*/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int fd1[2], fd2[2];
    pipe(fd1);
    pipe(fd2);

    int pid = fork();
    if (pid > 0)
    {
        close(fd1[0]); // 关闭父进程读端
        close(fd2[1]); // 关闭子进程写端

        char buf[3];
        write(fd1[1], "A", 1);
        read(fd2[0], &buf, 1);
        printf("%d: received pong\n", getpid());
    }
    else if (pid == 0)
    {
        char buf[3];
        printf("asdjfol 3\n");
        read(fd2[0], buf, 1);
        printf("asdjfol 23\n");
        printf("%d: received ping\n", getpid());
        printf("asdjfol 123\n");

        write(fd2[1], buf, 1);

        close(fd1[1]); // 关闭父进程写端
        close(fd2[0]); // 关闭子进程读端
    }

    exit(0);
}