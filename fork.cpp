#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(){
    pid_t pid = fork();
    if(pid==0){
        printf("子进程运行\n");
    }
    else{
        printf("父进程运行\n");
        wait(NULL);
        printf("父进程成功回收子进程\n");
    }
}