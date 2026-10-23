#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include "socket_until.h"

using namespace std;

void handle_client(int client_fd){

    string buf;
    while(true){
        buf.clear();
        ssize_t n=recv_message(client_fd,buf);
        if(n>0){
            cout<<"接收到客户端消息: "<<buf<<endl;
            const char* msg = "hello client";
            if (!send_message(client_fd, msg)) {
                cerr << "send failed: " << strerror(errno) << '\n';
            }
        }
        else if(n==0){
            cout<<"客户端关闭"<<endl;
            break;
        }
        else{
            cerr<<"recv failed"<<endl;
            break;
        }
    }
    close(client_fd);
}

int main(){
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd==-1){
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt=1;
    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1){
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    if(bind(server_fd,(sockaddr*)&addr,sizeof(addr))==-1){
        perror("bind");
        close(server_fd);
        return 1;
    }

    if(listen(server_fd,5)==-1){
        perror("listen");
        close(server_fd);
        return 1;
    }
    cout<<"开始监听8080端口"<<endl;

    while(true){
        int client_fd = accept(server_fd,nullptr,nullptr);
        if(client_fd==-1){
            perror("accept");
            close(server_fd);
            return 1;
        }
        cout<<"客户端链接成功"<<endl;
        thread(handle_client,client_fd).detach();
    }
    
    close(server_fd);
}