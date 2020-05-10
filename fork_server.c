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
#include "common.h"
#include "socketLib/socket_lib.h"

int response = OK_HTTP;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int setup_listen() {
  int socket_listen;

  if ((socket_listen = slisten(FORK_PORT)) < 0) 
  {
    error("Fork-Server: slisten");
  }

  return socket_listen;
}

/*
 * Process Terminated Child processes:
 */
void sigchld_handler(int signo)
{
    pid_t PID;
    int status;

    do
    {
        PID = waitpid(-1, &status, WNOHANG);
    } while (PID != -1);

    // Re-instate handler
    signal(SIGCHLD, sigchld_handler);
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
}

int main()
{
    int socket_server_fd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[BUFFER_SIZE];
    pid_t childpid;

    printf("INFO: Initializing FIFO server.\n");

    int socket_listen = setup_listen();

    // Set signal handler for SIGCHLD
    signal(SIGCHLD, sigchld_handler);

    while (1)
    {
        newSocket = accept(socket_listen, (struct sockaddr *)&newAddr, &addr_size);
        if (newSocket < 0)
        {
            exit(1);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
        childpid = fork();
        // Error
        if (childpid < 0)
        {
            close(newSocket);
            continue;
        }
        // Child
        else if (childpid == 0)
        {
            close(socket_listen);
            handle_request(newSocket);
            break;
        }
        // Parent
        else
        {
            close(newSocket);
            if (waitpid(childpid, NULL, 0) < 0)
            {
                perror("Failed to collect child process");
                break;
            }
            continue;
        };
    }

    close(newSocket);

    return 0;
}
