
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
using namespace std;

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // 第一个元素始终是监听 socket
    vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});
  
    cout << "poll 服务器正在监听 8080 端口" << endl;

    while (true) {
        int ret = poll(fds.data(),fds.size(),0);

        if(ret == -1){
            if(errno == EINTR)continue;
            close(server_fd);
            break;
        }
        // 先处理新连接
        
        if(fds[0].fd & POLLIN){
            int client_fd = accept(server_fd,nullptr,nullptr);
            if(client_fd<0){
                perror("accept");
            }
            else{
                fds.push_back({client_fd,POLLIN,0});
                cout<<"新客户端连接,fd= "<<client_fd<<endl;
            }
        }

        // 再处理已经连接的客户端
        for(size_t i=1;i<fds.size();){
            short event = fds[i].events;
            int client_fd = fds[i].fd;
            bool remove = false;
            if(event & POLLIN){
                char buf[1024];
                ssize_t n=recv(client_fd,buf,sizeof(buf)-1,0);
                
                if(n>0){
                    buf[n]='\0';
                    size_t sent = 0;

                    while(sent < n){
                        size_t x = send(client_fd , buf + sent , n-sent , MSG_NOSIGNAL);

                        if(x == 0){
                            perror("send");
                            remove = true;
                            break;
                        }
                        if(x==-1 && errno == EINTR)continue;

                        sent+=x;
                    }

                }
                else{
                    if (n == 0) {
                        cout << "客户端断开,fd = "<< client_fd << endl;
                    } 
                    else if (errno != EINTR) {
                        perror("recv");
                    }
                    remove = (n == 0 || errno != EINTR);
                }
            }
            // 客户端异常或连接挂断
            if(event & (POLLERR || POLLHUP || POLLNVAL)){
                remove = true;
            }
            if(remove){
                close(client_fd);
                fds.erase(fds.begin()+i);
            }
            else{
                i++;
            }
        }
        
    }

    for (size_t i = 1; i < fds.size(); ++i) {
        close(fds[i].fd);
    }
    close(server_fd);
    return 0;
}
