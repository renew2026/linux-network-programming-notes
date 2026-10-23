
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // 2. 创建 epoll 实例
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    // 3. 将监听 socket 加入 epoll
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        close(epfd);
        close(server_fd);
        return 1;
    }

    cout << "epoll 服务器正在监听 8080 端口" << endl;

    epoll_event events[64];

    // 4. 不断等待就绪事件
    while (true) {
        int n = epoll_wait(epfd,events,server_fd,0);
            
        if(n == -1){
            if(errno == EINTR)continue;
            perror("epoll_wait");
            break;
        }

        for(int i=0;i<n;i++){
            int fd = events[i].data.fd;
            uint32_t flags = events[i].events;

            if(fd == server_fd){
                int client_fd = accept(server_fd,nullptr,nullptr);

                if(client_fd == -1){
                    perror("accept");
                    continue;
                }

                epoll_event ev{};
                ev.data.fd = client_fd;
                ev.events = EPOLLIN | EPOLLRDHUP;
                if(epoll_ctl(epfd,EPOLL_CTL_ADD,client_fd,&ev) == -1){
                    perror("epoll_ctl client");
                    close(client_fd);
                    continue;
                }
                cout<<"新客户端连接,fd = "<<client_fd<<endl;
                continue;
            }

            if(flags & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
                epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
                close(fd);
                cout<<"客户端断开,fd = "<<fd<<endl;
                continue;
            }

            if(flags & (EPOLLIN)){

                char buf[1024];
                ssize_t bytes = recv(fd, buf, sizeof(buf), 0);

                if (bytes <= 0) {
                    if (bytes == -1) perror("recv");

                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    continue;
                }
                ssize_t sent = 0;
                bool ok = true;

                while (sent < bytes) {
                    ssize_t m = send(fd, buf + sent,
                                     bytes - sent, MSG_NOSIGNAL);

                    if (m == -1 && errno == EINTR) continue;

                    if (m <= 0) {
                        perror("send");
                        ok = false;
                        break;
                    }

                    sent += m;
                }
                if (!ok) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                }
            }

        }
    }

    close(epfd);
    close(server_fd);
    return 0;
}
