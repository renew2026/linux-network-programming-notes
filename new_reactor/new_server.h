#pragma once

#include "new_clients.h"
#include "new_epoll.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

class Server
{
private:
    // 单个客户端最大待发送数据
    static constexpr size_t MAX_PENDING = 4 * 1024 * 1024;

    // epoll 一次最多返回的事件数
    static constexpr int MAX_EVENTS = 128;

    // 监听 Socket
    int server_fd = -1;

    // 服务器端口
    uint16_t port;

    // epoll 管理对象
    Epoll epoll;

    // fd -> Client
    std::unordered_map<int, std::unique_ptr<Client>> clients;
    uint64_t next_token = 1;

    // token -> fd
    std::unordered_map<uint64_t, int> token_to_fd;

    // 服务器运行状态
    bool running = false;

private:
    // 创建监听 Socket
    std::string encodeMessage(const std::string &msg)
    {
        uint32_t len = htonl(static_cast<uint32_t>(msg.size()));

        std::string packet(sizeof(len), '\0');

        memcpy(packet.data(), &len, sizeof(len));

        packet += msg;

        return packet;
    }

    bool createListenSocket()
    {
        server_fd = socket(AF_INET, SOCK_CLOEXEC | SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (server_fd == -1)
        {
            perror("socket");
            return false;
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            perror("setsockopt");
            close(server_fd);
            server_fd = -1;
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("bind");
            close(server_fd);
            server_fd = -1;
            return false;
        }
        if (listen(server_fd, SOMAXCONN) == -1)
        {
            perror("listen");
            close(server_fd);
            server_fd = -1;
            return false;
        }
        return true;
    };

    // 接受新连接
    void acceptConnections()
    {
        while (true)
        {
            int client_fd = accept4(server_fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (client_fd == -1)
            {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                perror("accept4");
                break;
            }

            uint64_t token = next_token++;

            auto client = std::make_unique<Client>(client_fd, token);

            clients[client_fd] = std::move(client);
            token_to_fd[token] = client_fd;

            if (!epoll.add(client_fd, EPOLLIN | EPOLLRDHUP | EPOLLET, token))
            {
                token_to_fd.erase(token);
                clients.erase(client_fd);
                close(client_fd);
                continue;
            }
            std::cout << "新客户端接入, fd = " << client_fd << std::endl;
        }
    }

    // 更新客户端监听事件
    bool updateInterest(Client &client)
    {
        uint32_t events = EPOLLET;

        // 对端还没有关闭发送方向，继续监听读事件
        if (!client.isPeerEof())
        {
            events |= EPOLLIN | EPOLLRDHUP;
        }

        // 输出缓冲区有数据，才监听写事件
        if (client.outputBytes() > 0)
        {
            events |= EPOLLOUT;
        }

        return epoll.modify(client.getFd(), events, client.getToken());
    };

    // 删除客户端
    void removeClient(int fd)
    {
        auto it = clients.find(fd);

        if (it == clients.end())
            return;

        uint64_t token = it->second->getToken();

        epoll.remove(fd);
        token_to_fd.erase(token);

        close(fd);
        clients.erase(it);
    }

    // 处理客户端事件
    void handleClientEvent(int fd, uint32_t events)
    {
        auto it = clients.find(fd);
        if (it == clients.end())
        {
            return;
        }
        auto &c = *it->second;

        if (events & EPOLLERR)
        {
            int error = 0;
            socklen_t len = sizeof(error);

            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
                perror("getsockopt");
            else
                std::cerr << "客户端发生错误,fd = " << fd << " error = " << error << std::endl;
            removeClient(fd);
            return;
        }

        if (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))
        {
            IOStatus status = c.handleRead();

            if (status == IOStatus::ERROR)
            {
                removeClient(fd);
                return;
            }

            // 即使对端关闭了连接，也先处理已经收到的数据
            std::vector<std::string> messages;

            if (!c.extractMessages(messages))
            {
                c.markNeedClose();
            }
            else
            {
                handleMessages(fd, messages);
            }
            if (c.isPeerEof() && c.inputBytes() > 0)
            {
                c.markNeedClose();
            }
        }

        // 处理写事件
        if (events & EPOLLOUT)
        {
            IOStatus status = c.handleWrite();

            if (status == IOStatus::ERROR)
            {
                removeClient(fd);
                return;
            }
        }

        // 判断是否需要关闭
        if (c.shouldClose())
        {
            removeClient(fd);
            return;
        }

        // 更新 epoll 监听事件
        if (!updateInterest(c))
        {
            removeClient(fd);
            return;
        }
    };

    // 广播消息
    void broadcast(int from, const std::string &msg)
    {

        for (auto &[fd, c] : clients)
        {
            if (fd == from)
                continue;

            c->appendOutput(msg);

            bool ok = updateInterest(*c);
        }
    }

    // 处理客户端完整消息
    void handleMessages(int from, const std::vector<std::string> &messages)
    {
        for (const auto &msg : messages)
        {
            std::cout << "客户端 " << from << " 发送消息：" << msg << std::endl;

            std::string whole_msg = encodeMessage(msg);

            broadcast(from, whole_msg);
        }
    };

public:
    explicit Server(uint16_t port) : port(port) {};

    ~Server()
    {
        for (auto &[fd, client] : clients)
        {
            epoll.remove(fd);
            close(fd);
        }

        clients.clear();
        if (server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
    };

    // 禁止复制
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    // 启动服务器
    bool start()
    {
        if (epoll.getFd() == -1)
        {
            perror("epoll");
            return false;
        }

        if (!createListenSocket())
        {
            perror("create_server_failed");
            return false;
        }

        if (!epoll.add(server_fd, EPOLLIN | EPOLLET, 0))
        {
            perror("add server failed");
            close(server_fd);
            server_fd = -1;
            return false;
        }
        running = true;
        std::cout << "服务器创建成功,开始监听" << port << "窗口" << std::endl;
        return true;
    };

    // 事件循环
    void eventLoop()
    {
        epoll_event events[MAX_EVENTS];
        while (running)
        {
            int n = epoll.wait(events, MAX_EVENTS, 100);

            if (n == -1)
            {
                if (errno == EINTR)
                    continue;
                perror("epoll_wait");
                break;
            }
            for (int i = 0; i < n; i++)
            {
                uint64_t token = events[i].data.u64;
                uint32_t flags = events[i].events;

                if (token == 0)
                {
                    if (flags & EPOLLIN)
                    {
                        acceptConnections();
                    }
                    continue;
                }
                else
                {
                    auto it = token_to_fd.find(token);
                    if (it == token_to_fd.end())
                        continue;
                    int fd = it->second;
                    auto client_it = clients.find(fd);
                    if (client_it == clients.end())
                        continue;
                    if (client_it->second->getToken() != token)
                        continue;
                    handleClientEvent(fd, flags);
                }
            }
        }
    };

    // 停止服务器
    void stop()
    {
        running = false;
    };
};