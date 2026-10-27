
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "socket_until.h"

using namespace std;

constexpr int MAX_EVENTS = 128;
constexpr size_t MAX_PENDING = 1024 * 1024;

struct Client
{
    string out;      // 尚未发送的数据
    size_t sent = 0; // 已经发送的字节数
    bool need_close = false;
};

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1)
    {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl server");
        close(epfd);
        close(server_fd);
        return 1;
    }

    unordered_map<int, Client> clients;
    epoll_event events[MAX_EVENTS];

    cout << "非阻塞 epoll 服务器监听 8080 端口" << endl;

    auto remove_client = [&](int fd)
    {
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        clients.erase(fd);
    };

    auto update_interest = [&](int fd) -> bool
    {
        auto it = clients.find(fd);
        if (it == clients.end())
            return false;

        Client &c = it->second;

        epoll_event ev{};
        ev.data.fd = fd;
        ev.events = EPOLLET;

        if (!c.need_close)
            ev.events |= EPOLLIN | EPOLLRDHUP;

        if (c.sent < c.out.size())
            ev.events |= EPOLLOUT;

        // 如果已经不需要读，也没有待发送数据，就不应该继续等待事件

        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
        {
            perror("epoll_ctl MOD");
            return false;
        }

        return true;
    };

    auto send_pending = [&](int fd) -> bool
    {
        auto it = clients.find(fd);
        if (it == clients.end())
            return false;

        Client &c = it->second;

        while (c.sent < c.out.size())
        {
            ssize_t w = send(fd,
                             c.out.data() + c.sent,
                             c.out.size() - c.sent,
                             MSG_NOSIGNAL);

            if (w > 0)
            {
                c.sent += static_cast<size_t>(w);
            }
            else if (w == -1 && errno == EINTR)
            {
                continue;
            }
            else if (w == -1 &&
                     (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                break;
            }
            else
            {
                if (w == -1)
                    perror("send");
                // remove_client(fd);
                return false;
            }
        }

        if (c.sent == c.out.size())
        {
            c.out.clear();
            c.sent = 0;
        }
        else if (c.sent > 0)
        {
            c.out.erase(0, c.sent);
            c.sent = 0;
        }

        if (update_interest(fd))
            return true;
        else
        {
            return false;
        }
    };

    auto broadcast = [&](int from, const char *data, size_t len)
    {
        vector<int> to_close;

        for (auto &[fd, c] : clients)
        {
            if (fd == from || c.need_close)
                continue;

            size_t pending = c.out.size() - c.sent;

            if (pending > MAX_PENDING ||
                len > MAX_PENDING - pending)
            {
                to_close.push_back(fd);
                continue;
            }

            if (c.sent > 0)
            {
                c.out.erase(0, c.sent);
                c.sent = 0;
            }

            c.out.append(data, len);
            if (!update_interest(fd))
                to_close.push_back(fd);
        }

        for (int fd : to_close)
            remove_client(fd);
    };

    while (true)
    {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t flags = events[i].events;

            // 1. 监听 socket：处理所有待接受的连接
            if (fd == server_fd)
            {
                while (true)
                {
                    int client_fd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);

                    if (client_fd == -1)
                    {
                        if (errno == EINTR)
                            continue;
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                        {
                            break;
                        }
                        perror("accept4");
                        break;
                    }

                    epoll_event ev{};
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;

                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                    {
                        perror("epoll_stl add");
                        close(client_fd);
                        continue;
                    }

                    clients.emplace(client_fd, Client{});

                    cout << "接入新客户端,fd = " << client_fd << endl;
                }
                continue;
            }

            auto it = clients.find(fd);
            if (it == clients.end())
                continue;

            bool alive = true;
            Client &client = it->second;

            // 2. 发生错误时关闭连接
            if (flags & (EPOLLERR | EPOLLHUP))
            {
                client.need_close = true;
            }

            // 3. 有数据可读：一直读取，直到 EAGAIN
            if (flags & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))
            {
                char buf[4096];

                while (true)
                {
                    ssize_t r = recv(fd, buf, sizeof(buf), 0);

                    if (r > 0)
                    {
                        broadcast(fd, buf, static_cast<size_t>(r));
                    }
                    else if (r == 0)
                    {
                        client.need_close = true;
                        break;
                    }
                    else if (errno == EINTR)
                    {
                        continue;
                    }
                    else if (errno == EAGAIN ||
                             errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    else
                    {
                        perror("recv");
                        alive = false;
                        break;
                    }
                }
            }

            if (!alive)
            {
                remove_client(fd);
                continue;
            }

            // 4. 有待发送数据时，尝试立即发送
            if (!alive || !send_pending(fd))
            {
                remove_client(fd);
                continue;
            }

            if (client.need_close && client.out.empty())
            {
                remove_client(fd);
                continue;
            }
        }
    }

    for (const auto &[fd, client] : clients)
    {
        (void)client;
        close(fd);
    }

    close(epfd);
    close(server_fd);
    return 0;
}
