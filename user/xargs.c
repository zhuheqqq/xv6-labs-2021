//每一次读取一行，将该行所有空格替换为\0，这样命令就可以被分割。
//然后将argv[]指向这些命令。如果遇到换行符，执行fork，父进程等待子进程结束

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define STDIN 0

int main(int argc,char *argv[])
{
    char* Argv[MAXARG];
    int index=0;
    char buf[1024];
    char c;

    for(int i=1;i<=argc-1;i++)
    {
        Argv[i-1]=argv[i];//忽略程序名
    }

    while(1)
    {
        index=0;
        memset(buf,0,sizeof(buf));
        char* p=buf;
        int i=argc-1;
        while(1)
        {
            int num=read(STDIN,&c,1);
            if(num!=1)
            {
                exit(0);
            }

            if(c==' '||c=='\n')
            {
                buf[index++]='\0';
                Argv[i++]=p;
                p=&buf[index];
                if(c=='\n')
                {
                    break;
                }
            }else{
                buf[index++]=c;
            }
        }

        Argv[i]=0;
        int pid=fork();
        if(pid==0)
        {
            exec(Argv[0],Argv);
        }else{
            wait(0);
        }
    }
    exit(0);
}
