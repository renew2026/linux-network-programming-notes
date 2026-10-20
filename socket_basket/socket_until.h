#pragma once

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

using namespace std;

constexpr uint32_t MAX_MSG_SIZE = 1024 * 1024;

// 发送指定数量的字节
bool send_all(int fd, const void* data, size_t len) {
    const char* p = static_cast<const char*>(data);
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, MSG_NOSIGNAL);

        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        if (n == 0) return false;

        sent += static_cast<size_t>(n);
    }

    return true;
}

// 准确读取指定数量的字节
bool recv_exact(int fd, void* data, size_t len) {
    char* p = static_cast<char*>(data);
    size_t received = 0;

    while (received < len) {
        ssize_t n = recv(fd, p + received, len - received, 0);

        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        // 对端关闭连接，数据尚未收完整
        if (n == 0) return false;

        received += static_cast<size_t>(n);
    }

    return true;
}

// 发送一条长度前缀消息
bool send_message(int fd, const string& msg) {
    if (msg.size() > MAX_MSG_SIZE) return false;

    uint32_t len = htonl(static_cast<uint32_t>(msg.size()));

    return send_all(fd, &len, sizeof(len)) &&
           send_all(fd, msg.data(), msg.size());
}

// 接收一条长度前缀消息
bool recv_message(int fd, string& msg) {
    uint32_t net_len;

    if (!recv_exact(fd, &net_len, sizeof(net_len))) {
        return false;
    }

    uint32_t len = ntohl(net_len);

    if (len > MAX_MSG_SIZE) return false;

    msg.resize(len);

    if (len == 0) return true;

    return recv_exact(fd, &msg[0], len);
}
