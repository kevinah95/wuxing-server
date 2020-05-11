#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include "common.h"
#include "socketLib/socket_lib.h"

int response = OK_HTTP;

int childProcCount = 0;
struct sigaction signal_action, sa;
static bool stop_server = false;
static int client_socket, server_socket;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int setup_listen()
{
    int socket_listen;

    if ((socket_listen = slisten(FORK_PORT)) < 0)
    {
        error("Fork-Server: slisten");
    }

    return socket_listen;
}

void parent_handler()
{
  stop_server = true;
  printf("The server is going to close gracefully after next client request\n");
}

void child_handler()
{
    pid_t processID;
    /* Clean up all child zombies */
    while (childProcCount)
    {
        /* Non-blocking wait */
        processID = waitpid((pid_t)-1, NULL, WNOHANG);
        if (processID < 0)
        {
            perror("waitpid() failed");
            exit(1);
        }
        else if (processID == 0)
            break;
        else{
            childProcCount--;
        }
    }
}


void handle_request(int c_socket_fd)
{
    char *buffer = malloc(BUFFER_SIZE);
    char *aux_buffer = malloc(BUFFER_SIZE);
    int file;

    bzero(buffer, BUFFER_SIZE);

    if (recv(c_socket_fd, buffer, BUFFER_SIZE, 0) < 0)
    {
        free(buffer);
        printf("ERROR: It was not possible to receive message from client.\n\t%s", strerror(errno));
    }

    // Gets file name
    char *file_name;
    file_name = strtok(buffer, " ");
    file_name = strtok(NULL, " ");

    char aux_path[100];
    strcpy(aux_path, "");
    strcat(aux_path, SERVER_FILES);
    strcat(aux_path, file_name);

    response = OK_HTTP;

    if ((file = open(aux_path, O_RDONLY, "r")) == -1)
    {
        printf("The file %s does not exist.\nPlease note that the extension of the file is required.\n", aux_path);
        bzero(aux_path, sizeof(aux_path));
        strcpy(aux_path, SERVER_FILES);
        strcat(aux_path, FILE_NOT_FOUND);
        file = open(aux_path, O_RDONLY, "r");
        response = HTTP_NOT_FOUND;
    }

    char *header = malloc(BUFFER_SIZE);

    printf("Sending file: %s\n", aux_path);
    lseek(file, 0, SEEK_SET);
    bzero(header, BUFFER_SIZE);

    sprintf(header, HTTP_RESPONSE, response);

    if (send(c_socket_fd, header, strlen(header), 0) == -1)
    {
        printf("ERROR: Data sending failed.\n");
        free(header);
        close(file);
        return;
    }
    free(header);

    bzero(aux_buffer, BUFFER_SIZE);
    int chunk_length;
    while ((chunk_length = read(file, aux_buffer, BUFFER_SIZE)) > 0)
    {
        if (send(c_socket_fd, aux_buffer, chunk_length, 0) == -1)
        {
            printf("ERROR: Something went wrong while sending the file.\n");
            return;
        }
    }

    printf("File successfully sent.\n");
    close(file);
    free(aux_buffer);
    free(buffer);
    close(c_socket_fd);
}

int main()
{
    int socket_server_fd, ret;
    struct sockaddr_in serverAddr;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[BUFFER_SIZE];
    pid_t childpid;

    printf("INFO: Initializing Fork server.\n");

    server_socket = setup_listen();

    signal_action.sa_handler = child_handler;
    if (sigfillset(&signal_action.sa_mask) < 0)
    {
        perror("sigfillset() failed");
        exit(1);
    }
    /* SA_RESTART causes interrupted system calls to be restarted */
    signal_action.sa_flags = SA_RESTART;

    /* Set signal disposition for child-termination signals */
    if (sigaction(SIGCHLD, &signal_action, 0) < 0)
    {
        perror("sigfillset() failed");
        exit(1);
    }

    sa.sa_handler = parent_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    while (!stop_server)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&newAddr, &addr_size);
        if (client_socket < 0)
        {
            exit(1);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
        childpid = fork();
        // Error
        if (childpid < 0)
        {
            close(client_socket);
            continue;
        }
        // Child
        else if (childpid == 0)
        {
            close(server_socket);
            handle_request(client_socket);
            exit(0);
        }

        printf("with child process: %u\n", childpid);
        close(client_socket);
        childProcCount++;
    }

    printf("parent Stopped\n");

    return 0;
}


