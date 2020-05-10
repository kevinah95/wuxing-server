#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "socket_lib.h"

int saccept(int socket_server_fd) 
{
    struct sockaddr_in from;
    int from_lenght;

    int listener;

    if ((listener = accept(socket_server_fd, (struct sockaddr *)&from, &from_lenght)) < 0)
    {
        perror("ERROR: It was not possible to accept the request.\n");
    }
    return listener;
}