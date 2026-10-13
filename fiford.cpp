#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int main(){
    //int ret=mkfifo("./myfifo",0666);
    // if(ret==-1){
    //     perror("mkfifo");
    //     return 1;
    // }
    int fd=open("./myfifo",O_RDONLY,0666);
    char buf[128];
    int n=read(fd,buf,sizeof(buf)-1);
    buf[n]='\0';
    printf("进程读取到: %s\n",buf);//加一个while(true),可以持续化读取输入信息
    close(fd);
}
//该文件应搭配fifowr.cpp使用
