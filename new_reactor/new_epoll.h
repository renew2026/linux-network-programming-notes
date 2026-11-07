
#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <cstdint>
#include <cerrno>
#include <cstdio>

class Epoll
{
private:
    int epfd = -1;

public:
    // 创建 epoll 实例
    Epoll()
    {
        epfd = epoll_create1(EPOLL_CLOEXEC);

        if (epfd == -1)
        {
            perror("epoll_create1");
        }
    };

    // 析构，关闭 epoll fd
    ~Epoll()
    {
        if (epfd != -1)
        {
            close(epfd);
            epfd = -1;
        }
    };

    // 禁止复制
    Epoll(const Epoll &) = delete;
    Epoll &operator=(const Epoll &) = delete;

    // 添加监听
    bool add(int fd, uint32_t events, uint64_t token)
    {
        epoll_event ev{};

        ev.data.u64 = token;
        ev.events = events;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
        {
            perror("epoll_ctl add");
            return false;
        }
        return true;
    };

    // 修改监听事件
    bool modify(int fd, uint32_t events, uint64_t token)
    {
        epoll_event ev{};

        ev.data.u64 = token;
        ev.events = events;

        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
        {
            perror("epoll_ctl modify");
            return false;
        }
        return true;
    };

    // 删除监听
    bool remove(int fd)
    {
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1)
        {
            if (errno == ENOENT || errno == EBADF)
            {
                return true;
            }
            perror("epoll_ctl delete");
            return false;
        }
        return true;
    };

    // 等待事件
    size_t wait(epoll_event *events, int max_events, int timeout)
    {
        size_t n = epoll_wait(epfd, events, max_events, timeout);
        return n;
    };

    // 获取 epoll fd
    int getFd() const
    {
        return epfd;
    }
};
