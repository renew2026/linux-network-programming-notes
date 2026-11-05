#include "Buffer.h"
#include "Clients.h"
#include "epoll.h"
#include "server.h"

int main()
{
    Server server(8080);
    if (!server.start())
    {
        perror("failed");
        return 1;
    }
    server.eventLoop();
    return 0;
}