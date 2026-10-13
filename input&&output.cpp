#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;

int main(){
    int saved=dup(1);
    int fd=open("demo.txt",O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd<0){
        perror("failed demo");
        return 1;
    }
    cout<<"成功打开|创建demo.txt,fd为: "<<fd<<endl;//当前cout的输出对象仍是终端
    const char* str="hello linux!\n";
    write(fd,str,strlen(str));//当前可以只使用fd进行输入
    int fd2=dup(fd);//先利用dup复制一份fd
    if(fd2<0){
        perror("dup");
        close(fd);
        return 1;
    }
    const char* str2="hello world!\n";
    write(fd2,str2,strlen(str2));//检查一下fd2是否有效
    close(fd);
    dup2(fd2,1);
    cout<<"nice to meet you!\n"<<endl;//检查是否重写成功
    dup2(saved,1);
    close(fd2);
}