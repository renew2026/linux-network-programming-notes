#pragma once

#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

using namespace std;

class Buffer
{
private:
    string data;
    size_t read_pos = 0;

public:
    void append(const char *buf, int len)
    {
        data.append(buf, len);
    } // 追加数据

    void append(const string &msg)
    {
        data += msg;
    }

    size_t readableBytes() const
    {
        return data.size() - read_pos;
    }
    // 返回未读取的数据长度

    const char *peek() const
    {
        return &data[(int)read_pos];
    }
    // 返回当前可读取数据的起始地址。

    void retrieve(size_t len)
    {
        if (len > readableBytes())
        {
            throw out_of_range("Buffer::retrieve out of range");
        }

        read_pos += len;
    }
    // 消费前 len 个字节，不能越界

    void clear()
    {
        data.clear();
        read_pos = 0;
    }
    // 清空缓冲区并重置位置
    void compact()
    {
        if (read_pos == 0)
            return;

        data.erase(0, read_pos);
        read_pos = 0;
    }
    // 清理缓冲区
};