#include <iostream>
#include <cstring>
#include <cerrno>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        cerr << "connect failed: " << strerror(errno) << endl;
        close(fd);
        return 1;
    }

    cout << "成功连接到服务器.." << endl;

    atomic<bool> running = true;

    // 接收线程：随时打印服务器广播来的消息
    auto work = [&]()
    {
        char buf[4096];
        while (running)
        {
            ssize_t r = recv(fd, buf, sizeof(buf), 0);
            if (r > 0)
            {
                cout << "\n[收到] " << string(buf, r) << "\n> " << flush;
            }
            else
            {
                if (r == 0)
                    cout << "\n服务器关闭了连接\n";
                else
                    perror("recv");
                running = false;
                break;
            }
        }
    };
    thread recv_thread(work);

    // 主线程：读用户输入并发送
    string line;
    while (running)
    {
        cout << "> " << flush;
        if (!getline(cin, line))
            break;

        if (line == "quit")
            break;

        if (send(fd, line.data(), line.size(), 0) == -1)
        {
            perror("send");
            break;
        }
    }

    running = false;
    shutdown(fd, SHUT_RDWR); // 让 recv 线程退出
    recv_thread.join();
    close(fd);
    return 0;
}