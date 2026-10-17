#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

using namespace std;

int main(){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket");
        return 1;
    }
    cout<<"socket创建成功,fd= "<<fd<<endl;
    close(fd);
}