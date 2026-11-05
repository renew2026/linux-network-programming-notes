
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

constexpr uint32_t MAX_MESSAGE_SIZE = 1024 * 1024;

bool sendAll(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len)
    {
        ssize_t n = send(fd, buf + sent, len - sent, MSG_NOSIGNAL);

        if (n > 0)
            sent += static_cast<size_t>(n);
        else if (n == -1 && errno == EINTR)
            continue;
        else
        {
            perror("send");
            return false;
        }
    }

    return true;
}

bool sendMessage(int fd, const string &msg)
{
    if (msg.size() > MAX_MESSAGE_SIZE)
    {
        cerr << "消息过长" << endl;
        return false;
    }

    uint32_t len = htonl(static_cast<uint32_t>(msg.size()));

    string packet(sizeof(len), '\0');
    memcpy(packet.data(), &len, sizeof(len));
    packet += msg;

    return sendAll(fd, packet.data(), packet.size());
}

bool extractMessages(string &buffer)
{
    size_t pos = 0;

    while (buffer.size() - pos >= sizeof(uint32_t))
    {
        uint32_t len = 0;

        memcpy(&len, buffer.data() + pos, sizeof(len));
        len = ntohl(len);

        if (len > MAX_MESSAGE_SIZE)
        {
            cerr << "收到非法消息长度: " << len << endl;
            return false;
        }

        size_t total = sizeof(uint32_t) + static_cast<size_t>(len);

        if (buffer.size() - pos < total)
            break;

        string msg(buffer.data() + pos + sizeof(uint32_t), len);

        cout << "\n[收到] " << msg << "\n> " << flush;

        pos += total;
    }

    if (pos > 0)
        buffer.erase(0, pos);

    return true;
}

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
        perror("connect");
        close(fd);
        return 1;
    }

    cout << "成功连接到服务器" << endl;

    atomic<bool> running = true;

    auto work = [&]()
    {
        char buf[4096];
        string buffer;

        while (running)
        {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);

            if (n > 0)
            {
                buffer.append(buf, static_cast<size_t>(n));

                if (!extractMessages(buffer))
                {
                    running = false;
                    shutdown(fd, SHUT_RDWR);
                    break;
                }
            }
            else if (n == 0)
            {
                cout << "\n服务器关闭了连接" << endl;
                running = false;
                break;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else
            {
                if (running)
                    perror("recv");

                running = false;
                break;
            }
        }
    };
    thread recv_thread(work);

    string line;

    while (running)
    {
        cout << "> " << flush;

        if (!getline(cin, line))
            break;

        if (line == "quit")
            break;

        if (!sendMessage(fd, line))
        {
            running = false;
            break;
        }
    }

    running = false;

    shutdown(fd, SHUT_RDWR);

    recv_thread.join();

    close(fd);

    return 0;
}