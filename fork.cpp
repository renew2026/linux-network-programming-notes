#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(){
    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }
    if(child_pid == 0){
        printf("子进程运行\n");
        sleep(1);
        printf("子进程结束\n");
    }
    else{
        printf("子进程pid为: %d\n",child_pid);
        pid_t parent_pid = getpid();
        printf("父进程pid为: %d\n",parent_pid);
        printf("父进程运行\n");
        wait(NULL);//必须等父进程回收子进程以后再结束，防止出现zombie
        printf("父进程成功回收子进程\n");
    }
}
