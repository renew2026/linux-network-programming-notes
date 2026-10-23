
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "socket_until.h"

using namespace std;

int main() {
    // 1. 创建监听 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    // 2. 绑定地址和端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // 3. 开始监听
    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // 记录已经连接的客户端
    set<int> clients;

    cout << "服务器正在监听 8080 端口" << endl;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);

        // 监听新连接
        FD_SET(server_fd,&readfds);
        int max_fd = server_fd;

        // 监听所有客户端
        for(int fd : clients){
            FD_SET(fd,&readfds);
            max_fd = max(max_fd , fd);
        }

        // 4. 等待事件发生
        int ret = select(max_fd+1, &readfds,nullptr,nullptr,nullptr);
        if(ret == -1){
            if(errno == EINTR)continue;
            perror("select");
            break;
        }

        // 5. 有新客户端连接
        if(FD_ISSET(server_fd,&readfds)){
            int client_fd=accept(server_fd,nullptr,nullptr);
            if(client_fd != -1){
                clients.insert(client_fd);
                cout<<"新连接客户端,fd= "<<client_fd<<endl;
            }
        }
        // 6. 检查客户端是否有数据
        for(auto it=clients.begin();it!=clients.end();){
            int fd=*it;
            if(!FD_ISSET(fd,&readfds)){
                it++;
                continue;
            }
            string buf;
            int n=recv_message(fd,buf);
            
            if(n<=0){
                if(n==0){
                    cout<<"客户端断开,fd= "<<fd<<endl;
                }
                else{
                    perror("recv");
                }
                close(fd);
                it=clients.erase(it);
                continue;
            }
            if(!send_message(fd,buf)){
                perror("send");
                close(fd);
                it=clients.erase(it);
                goto nex;
            }
            it++; 
            nex:;
        }   
    }

    for (int fd : clients) close(fd);
    close(server_fd);
    return 0;
}
