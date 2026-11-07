#include "new_Buffer.h"
#include "new_clients.h"
#include "new_epoll.h"
#include "new_server.h"

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