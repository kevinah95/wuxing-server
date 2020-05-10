#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "socket_lib.h"

int slisten(int port)
{
    struct sockaddr_in server;
    int socket_server_fd;

    int on = 1;

    socket_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server_fd < 0)
    {
        perror("ERROR: It was impossible to open the server.\n");
    }

    // Allow server to reuse address when binding
    if (setsockopt(socket_server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
    }
    
    // Allows UDP and TCP to reuse port (and address) when binding
    if (setsockopt(socket_server_fd, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_REUSEPORT");
    }

    bzero(&server, sizeof(server));

    // Information required by the server
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Port and socket are bind
    if (bind(socket_server_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("ERROR: It was not possible to assign an address to the server.\n");
    }

    // Listen requests
    if (listen(socket_server_fd, QUEUE_LISTENING_SIZE) < 0)
    {
        perror("ERROR: It is not possible to listen in the assigned port.\n");
    }

    return socket_server_fd;
}