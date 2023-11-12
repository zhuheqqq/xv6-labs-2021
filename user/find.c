#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* name;

void find(char* path)
{
    int fd;
    struct dirent de;
    struct stat st;
    //char buf[512];
    char* p=path+strlen(path);

    if((fd=open(path,0))<0)
    {
        fprintf(2,"find: cannot open %s\n",path);
        return;
    }

    if(fstat(fd,&st)<0)//获取文件元数据信息
    {
        fprintf(2,"find: cannot stat %s\n",path);
        close(fd);
        return;
    }

    *p++='/';

    while(read(fd,&de,sizeof(de))==sizeof(de))//获得目录下所有文件/子目录的结构体
    {
        if(strcmp(de.name,".")==0||strcmp(de.name,"..")==0||de.inum==0)
        {
            continue;
        }

        //strcpy(buf,path);
        
        strcpy(p,de.name);

        if(stat(path,&st)<0)
        {
            printf("find: cannot stat %s\n", p);
            continue;
        }

        if(st.type==T_DIR)
        {
            find(path);
        }else if(strcmp(p,name)==0)
        {
            printf("%s\n",path);
        }
    }

    close(fd);
    p=p-1;
    memset(p,'\0',strlen(p));
}

int main(int argc,char* argv[])
{
    if(argc==1)
    {
        fprintf(2,"find: no use arguments...\n");
        exit(1);
    }
    char path[512]={};
    if(argc==2)
    {
        path[0]='.';
        name=argv[1];
    }
    if(argc==3)
    {
        strcpy(path,argv[1]);
        name=argv[2];
    }

    find(path);

    return 0;
}