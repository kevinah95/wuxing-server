#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/mman.h>
#include "common.h"
#include "socketLib/socket_lib.h"

struct sockaddr_in server, client; // Handle the sockets' address

unsigned int client_lenght;

int socket_server_fd, on = 1; // Server file descriptors

int response = OK_HTTP;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int setup_listen() {
  int socket_listen;

  if ((socket_listen = slisten(THREAD_PORT)) < 0) 
  {
    error("FIFO-Server: slisten");
  }

  return socket_listen;
}

void *handle_request(void *c_socket)
{
    int *socket = (int *)c_socket;
    int client_socket = *socket;

    char *buffer = malloc(BUFFER_SIZE);
    char *aux_buffer = malloc(BUFFER_SIZE);
    int file;

    bzero(buffer, BUFFER_SIZE);
    if (recv(client_socket, buffer, BUFFER_SIZE, 0) <= 0)
    {
        free(buffer);
        error("ERROR: It was not possible to receive the message from client.\n");
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

    if (send(client_socket, header, strlen(header), 0) == -1)
    {
        printf("ERROR: Data sending failed.\n");
        free(header);
        close(file);
    }
    free(header);

    bzero(aux_buffer, BUFFER_SIZE);
    int chunk_length;
    while ((chunk_length = read(file, aux_buffer, BUFFER_SIZE)) > 0)
    {
        if (send(client_socket, aux_buffer, chunk_length, 0) == -1)
        {
            printf("ERROR: Something went wrong while sending the file.\n");
        }
    }

    printf("File successfully sent.\n");
    close(file);
    free(aux_buffer);
    free(buffer);
    pthread_exit(0);
}

void listen_requests(int socket_listen)
{
    pthread_t threads;
    while (1)
    {
        client_lenght = sizeof(client);
        int *client_socket = malloc(sizeof(int));

        if ((*client_socket = accept(socket_listen, (struct sockaddr *)&client, &client_lenght)) < 0)
        {
            printf("ERROR: It was not possible to accept the request.\n");
        }
        else
        {
            printf("Attending client: %s\n", inet_ntoa(client.sin_addr));

            if ((pthread_create(&(threads), NULL, handle_request, client_socket)) != 0)
            {
                close(socket_listen);
                error("ERROR: Error creating pthread.\n");
            }

            pthread_join(threads, NULL);
            close(*client_socket);
            free(client_socket);
        }
    }
}

int main(int argc, char *argv[])
{
    printf("INFO: Initializing THREAD server.\n");
    int socket_listen = setup_listen();
    listen_requests(socket_listen);
    return 0;
}