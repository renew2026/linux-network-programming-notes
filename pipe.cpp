#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    int pipefd1[2],pipefd2[2];//我们要建立双向管道
    if(pipe(pipefd1)==-1){
        perror("pipefd1");
        return 1;
    }
    if(pipe(pipefd2)==-1){
        perror("pipefd2");
        return 1;
    }
    pid_t pid = fork();
    if(pid==0){
        close(pipefd1[1]);
        close(pipefd2[0]);

        char buf[128];
        int n=read(pipefd1[0],buf,sizeof(buf)-1);
        buf[n]='\0';//需要手动添加\0,否则会出现乱码
        printf("子进程读取信息: %s\n",buf);//先读取父进程发来的消息
        close(pipefd1[0]);//必须关闭,否则一直等待父进程发信息

        const char* str2="hello world!\n";
        write(pipefd2[1],str2,strlen(str2));
        close(pipefd2[1]);//用完就关
    }
    else{
        close(pipefd1[0]);
        close(pipefd2[1]);

        const char* str1="hello linux!\n";
        write(pipefd1[1],str1,strlen(str1));
        close(pipefd1[1]);

        char buf[128];
        int n=read(pipefd2[0],buf,sizeof(buf)-1);
        buf[n]='\0';
        printf("父进程读取信息: %s\n",buf);
        close(pipefd2[0]);

        wait(NULL);
        printf("父进程成功回收子进程\n");
    }
}