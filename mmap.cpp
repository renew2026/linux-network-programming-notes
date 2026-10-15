#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

int main(){
    int* p=(int*)mmap(
        nullptr,
        sizeof(int),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );
    *p=10;
    pid_t pid = fork();
    if(pid<0){
        perror("fork");
        return 1;
    }
    
    if(pid == 0){
        printf("子进程接收值为: %d\n",*p);
        
        *p=100;

        printf("子进程修改值为: %d\n",*p);
    }
    else{
        printf("父进程初始值为: %d\n",*p);

        *p=50;

        printf("父进程接收值为: %d\n",*p);
        wait(NULL);

        printf("最终值为: %d\n",*p);
    }
}