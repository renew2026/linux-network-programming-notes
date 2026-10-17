#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main(){
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1){
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "connect failed: " << strerror(errno) << endl;
        close(fd);
        return 1;
    }
    cout << "成功连接到服务器.." <<endl;

    const char* msg = "hello linux";
    if (send(fd, msg, strlen(msg), 0) == -1) {
        cerr << "send failed: " << strerror(errno) << endl;
        close(fd);
        return 1;
    }

    char buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n > 0) {
        cout << "收到服务端回复: ";
        cout.write(buf, n);
        cout << endl;
    } else if (n == 0) {
        cout << "服务端关闭了连接" << endl;
    } else {
        cerr << "recv failed: " << strerror(errno) << endl;
    }

    close(fd);
}