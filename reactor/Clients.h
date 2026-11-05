#pragma once

#include "Buffer.h"

#include <string>
#include <vector>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <arpa/inet.h>
#include <utility>

enum class IOStatus
{
    OK,          // 正常，或者已经读到 EAGAIN
    PEER_CLOSED, // 对端关闭了写方向
    ERROR        // 发生致命 IO 错误
};

class Client
{
private:
    int fd;

    Buffer input;
    Buffer output;

    bool peer_eof = false;
    bool need_close = false;

public:
    explicit Client(int socket_fd) : fd(socket_fd) {};

    ~Client() = default;

    // 禁止复制客户端对象
    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

    // 获取 Socket
    int getFd() const
    {
        return fd;
    }

    // 接收数据，循环读取直到 EAGAIN
    IOStatus handleRead()
    {
        char buf[4096];
        while (true)
        {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n > 0)
            {
                input.append(buf, n);
            }
            else if (n == 0)
            {
                peer_eof = true;
                return IOStatus::PEER_CLOSED;
            }
            else if (n == -1 && errno == EINTR)
            {
                continue;
            }
            else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                break;
            }
            else
            {
                perror("recv");
                need_close = true;
                return IOStatus::ERROR;
            }
        }
        return IOStatus::OK;
    }

    // 发送数据，循环发送直到 EAGAIN 或发送完成
    IOStatus handleWrite()
    {
        while (output.readableBytes() > 0)
        {
            ssize_t n = send(fd, output.peek(), output.readableBytes(), MSG_NOSIGNAL);

            if (n > 0)
            {
                output.retrieve(static_cast<size_t>(n));
            }
            else if (n == 0)
            {
                need_close = true;
                return IOStatus::ERROR;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return IOStatus::OK;
            }
            else
            {
                perror("send");
                need_close = true;
                return IOStatus::ERROR;
            }
        }

        return IOStatus::OK;
    }

    bool extractMessages(std::vector<std::string> &messages)
    {
        const uint32_t MAX_MESSAGE_SIZE = 4 * 1024 * 1024; // 4MB，防止恶意超大长度

        while (input.readableBytes() >= 4)
        {
            uint32_t len = 0;
            memcpy(&len, input.peek(), sizeof(len));
            len = ntohl(len);

            if (len > MAX_MESSAGE_SIZE)
            {
                need_close = true;
                return false;
            }

            if (input.readableBytes() < 4 + static_cast<size_t>(len))
            {
                break;
            }

            input.retrieve(4);
            std::string msg(input.peek(), len);
            input.retrieve(len);
            messages.push_back(std::move(msg));
        }

        return true;
    };

    // 添加待发送数据
    void appendOutput(const std::string &msg)
    {
        output.append(msg);
    }

    // 获取待读取和待发送数据长度
    size_t inputBytes() const
    {
        return input.readableBytes();
    };
    size_t outputBytes() const
    {
        return output.readableBytes();
    };

    // 连接状态
    bool isPeerEof() const
    {
        return peer_eof;
    };
    bool shouldClose() const
    {
        return (need_close || (peer_eof && output.readableBytes() == 0));
    };

    // 标记关闭状态
    void markPeerEof()
    {
        peer_eof = true;
    };
    void markNeedClose()
    {
        need_close = true;
    };
};