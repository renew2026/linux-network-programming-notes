#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int main(){
    int ret=mkfifo("./myfifo",0666);
    if(ret==-1){
        perror("mkfifo");
        return 1;
    }
    int fd=open("./myfifo",O_WRONLY,0666);
    const char* str="hello linux!\n";
    write(fd,str,strlen(str));
    close(fd);
}
//该文件应搭配fiford.cpp使用,如果不搭配,进程就会直接卡死
