#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct sockaddr_in server, client, cli_addr; // Handle the sockets' address

int server_fd, client_socket; // Client and server file descriptors

int response = OK_HTTP;

int number_of_threads;
pthread_t * pthreads;

typedef struct
{
    sem_t empty, full, mutex;
    int buffer[1];
} SHARED;

int buffer_shm_fd;
SHARED *shared;

void set_shared_memory() 
{
    if ((buffer_shm_fd = shm_open("shared_buffer", O_CREAT | O_RDWR, 0666)) == -1)
    {
        fprintf(stderr, "Open failed:%s\n", strerror(errno));
        exit(1);
    }
    ftruncate(buffer_shm_fd, sizeof(SHARED));

    if ((shared = mmap(0, sizeof(SHARED), PROT_READ | PROT_WRITE, MAP_SHARED, buffer_shm_fd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "mmap failed : %s\n", strerror(errno));
        exit(1);
    }
}

void get_shared()
{
    int buffer_shm_fd;

    // Open the shared memory segment
    buffer_shm_fd = shm_open("shared_buffer", O_RDWR, 0666);
    ftruncate(buffer_shm_fd, sizeof(SHARED));

    // Now map the shared memory segment of the buffer in the address space of the process
    shared = mmap(0, sizeof(SHARED), PROT_READ | PROT_WRITE, MAP_SHARED, buffer_shm_fd, 0);
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void init_semaphores()
{
    set_shared_memory();
    sem_init(&shared->empty, 1, 1);
    sem_init(&shared->full, 1, 0);
    sem_init(&shared->mutex, 1, 1);
}

void handle_request(int c_socket)
{
    char *buffer = malloc(BUFFER_SIZE);
    char *aux_buffer = malloc(BUFFER_SIZE);
    int file;

    bzero(buffer, BUFFER_SIZE);
    int result;
    
    result = recv(c_socket, buffer, BUFFER_SIZE, 0);

    if (result < 0)
    {
        free(buffer);
        fprintf(stderr, "ERROR: It was not possible to receive message from client: %s\n", strerror(errno));
        exit(1);
        //error("ERROR: It was not possible to receive message from client.\n");
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

    if (send(c_socket, header, strlen(header), 0) == -1)
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
        if (send(c_socket, aux_buffer, chunk_length, 0) == -1)
        {
            printf("ERROR: Something went wrong while sending the file.\n");
            return;
        }
    }

    printf("File successfully sent.\n");
    close(file);
    free(aux_buffer);
    free(buffer);
    close(c_socket);
}

void * listen_requests(void* thread_index)
{
    int thread_ind = *((int*)thread_index);
    unsigned int client_lenght;

    pthread_detach(pthread_self());
    get_shared();

    while(1) 
    {
        sem_wait(&shared->full); 
        sem_wait(&shared->mutex); 

        server_fd = shared->buffer[0];

        client_lenght = sizeof(client);

        if ((client_socket = accept(server_fd, (struct sockaddr *)&client, &client_lenght)) < 0)
        {
            printf("ERROR: It was not possible to accept the request.\n");
        }
        else
        {
            printf("Attending client: %s\n", inet_ntoa(client.sin_addr));
            handle_request(client_socket);
        }

        sem_post(&shared->mutex); 
        sem_post(&shared->empty);
    }
}

void init()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd < 0)
    {
        error("ERROR: It was impossible to open the Server.\n");
    }
    bzero(&server, sizeof(server));

    // Information required by the server
    server.sin_port = htons(PRE_THREAD_PORT);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Port and socket are bind
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        error("ERROR: bind() failed. It was not possible to assign an address to the server.\n");
    }

    // Listen requests
    if (listen(server_fd, 100) < 0)
    {
        error("ERROR: listen() failed. It is not possible to listen in the assigned port.\n");
    }

    for (int i = 0; i < number_of_threads; i++){
        if ((pthread_create(&(pthreads[i]), NULL, listen_requests, &i)) != 0)
        {
            printf("ERROR: Failure creating the thread %d\n", i);
            exit(-1);
        }
    }

    init_semaphores();
    get_shared();
    while(1)
    {
        sem_wait(&shared->empty);
        sem_wait(&shared->mutex);
        
        shared->buffer[0] = server_fd;
        
        sem_post(&shared->mutex);
        sem_post(&shared->full);
    }
    
    for (int i = 0; i < number_of_threads; i++)
    {
        pthread_kill(&pthreads[i], 0);
    }
    free(pthreads);
}

void pthreads_factory(int argc, char * argv[])
{
    if (argc != 2)
    {
        error("ERROR: Please add the number of threads that the server will run.\n");
    }
    else
    {
        int num_of_threads = atoi(argv[1]);
        if (num_of_threads < 1){
            error("ERROR: At least one thread is required.\n");
        }
        number_of_threads = num_of_threads;
        
        pthreads = calloc(number_of_threads, sizeof(pthread_t));
    }
}

int main(int argc, char *argv[])
{
    printf("INFO: Initializing PRE-THREADED server.\n");
    pthreads_factory(argc, argv);
    init();
    return 0;
}