#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/file.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include "common.h"

struct sockaddr_in server; // Handle the socket address

int socket_server_fd; // Server file descriptor

char *ip; // Server IP address

char *get_http = "GET %s HTTP/2.0\n"; // Get HTTP request

char *requested_file; // File requested by client

char *path = "./client_files/"; // Folder where client files will be saved

pthread_t thread; // pthread that's going to be created to perform client requests
unsigned int threads = 0; // Number of threads that will be executed
unsigned int cycles = 0; // Number of cycles a thread will repeat a request
unsigned short port = 0; // Server's port

int total_client_requests = 0;
int total_files_transfered = 0;
long old_time_in_microseconds = 0;
long new_time_in_microseconds = 0;
long old_short_period_time = 0;
long new_short_period_time = 0;
int num_times = 0;
long times[256];
int l = 0;
int file_output = 0;

// Contain info required by the thread: server_socket to connect, and message to send
struct thread_info_st
{
    int server_socket;
    char *message;
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void receive_arguments(int argc, char *argv[])
{
    if (argc != 6)
    {
        error("ERROR: Wrong number/format of the parameters.\nThe expected format is: ./client <ip> <port> <file> <N-threads> <N-cycles>");
    }
    ip = argv[1];
    port = atoi(argv[2]);
    requested_file = argv[3];
    threads = atoi(argv[4]);
    cycles = atoi(argv[5]);
    total_client_requests = atoi(argv[4]) * atoi(argv[5]);
     num_times = cycles * threads;
     l = num_times - 1;
}

void *get(void *threads_st)
{
    struct thread_info_st *thread = (struct thread_info_st *)threads_st;


    for (int i = 0; i < cycles; i++)
    {
        gettimeofday(&shortOldTime, NULL);
        old_short_period_time = (int64_t)(shortOldTime.tv_sec) * (int64_t)1000000000 + (int64_t)(shortOldTime.tv_nsec);
        struct thread_info_st cycle_thread = thread[i];
        int cycle_socket = cycle_thread.server_socket;
        char *cycle_message = cycle_thread.message;

        char message_to_server[BUFFER_SIZE];
        char transfer_buffer[BUFFER_SIZE];
        bzero(message_to_server, BUFFER_SIZE);
        sprintf(message_to_server, get_http, cycle_message);
        if (sendto(cycle_socket, message_to_server, strlen(message_to_server), 0, (struct sockaddr *)&server, sizeof(server)) != strlen(message_to_server))
        {
            error("ERROR: The sum of sent bytes is not the expected.\n");
        }

        bzero(transfer_buffer, BUFFER_SIZE);
        int received;
        // Read message from position 9 ignoring 'HTTP/2.0'
        if ((received = recv(cycle_socket, transfer_buffer, 9, 0)) < 0)
        {
            error("ERROR: There was an error in the transfer of data or the connection was lost.\n");
        }

        bzero(transfer_buffer, BUFFER_SIZE);
        // Read HTTP message code: 200 or 404
        if ((received = recv(cycle_socket, transfer_buffer, 3, 0)) < 0)
        {
            error("ERROR: There was an error in the transfer of data or the connection was lost.\n");
        }

        // 404 means file does not exist on server
        if (strcmp(transfer_buffer, "404") == 0)
        {
            printf("File %s does not exist on server.\n", cycle_message);
            strcpy(cycle_message, FILE_NOT_FOUND);
            file_output = 1;
        }
        bzero(transfer_buffer, BUFFER_SIZE);
        char *aux_buffer;

        if ((received = recv(cycle_socket, transfer_buffer, BUFFER_SIZE, 0)) <= 0)
        {
            error("ERROR: There was an error in the transfer of data or the connection was lost.\n");
        }

        // Move to the beginning of the file data
        aux_buffer = strstr(transfer_buffer, "\r\n\n");
        aux_buffer = &aux_buffer[3];
        int file;

        char full_path[100];
        strcpy(full_path, path);
        strcat(full_path, cycle_message);

        // File is created in client's folder
        if ((file = creat(full_path, 0666)) == -1)
        {
            printf("ERROR: It was not possible to create file %s.\n", cycle_message);
            exit(1);
        }

        // Data from buffer is written in the file
        if ((write(file, aux_buffer, sizeof(char) * (received + 3 - (sizeof(char) * (3 + ((int)aux_buffer - (int)transfer_buffer)))))) < 0)
        {
            printf("ERROR: It was not possible to write on file %s.\n", cycle_message);
            exit(1);
        }

        bzero(transfer_buffer, BUFFER_SIZE);
        while ((received = recv(cycle_socket, transfer_buffer, BUFFER_SIZE, 0)) > 0)
        {
            if ((write(file, transfer_buffer, sizeof(char) * received)) < 0)
            {
                printf("ERROR: It was not possible to write on file %s.\n", cycle_message);
                exit(1);
            }
            bzero(transfer_buffer, BUFFER_SIZE);
        }
        close(file);
        gettimeofday(&shortNewTime, NULL);
        new_short_period_time = (int64_t)(shortNewTime.tv_sec) * (int64_t)1000000000 + (int64_t)(shortNewTime.tv_nsec);
        times[l] = new_short_period_time - old_short_period_time;
        total_files_transfered += 1;
        printf("File transfer ended.\n");
        l = l - 1;
    }
}

// Create the connection with the server
void connect_to_server()
{
    int index = 0;

    struct thread_info_st thread_data[cycles];

    do
    {
        for (int i = 0; i < cycles; i++)
        {
            if ((socket_server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            {
                error("ERROR: It was not possible to establish connection with the socket.\n");
            }

            bzero(&server, sizeof(server));
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(ip);
            server.sin_port = htons(port);

            if (connect(socket_server_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
            {
                error("ERROR: Connection with server failure.\n");
            }
            thread_data[i].server_socket = socket_server_fd;
            thread_data[i].message = requested_file;
        }

        if (pthread_create(&thread, NULL, get, (void *)thread_data) != 0)
        {
            error("ERROR: It was not possible to create a pthread.\n");
        }
        pthread_join(thread, NULL);

        index++;
    } while (index < threads);

    close(socket_server_fd);
}

int set_old_time(){
    gettimeofday(&oldTime, NULL);
    old_time_in_microseconds = (int64_t)(oldTime.tv_sec) * (int64_t)1000000000 + (int64_t)(oldTime.tv_nsec);
}

int set_new_time(){
    gettimeofday(&newTime, NULL);
    new_time_in_microseconds = (int64_t)(newTime.tv_sec) * (int64_t)1000000000 + (int64_t)(newTime.tv_nsec);
}

int server_statistics(){
    total_files_transfered = total_files_transfered * 100 / total_client_requests;
    long total_time_request = new_time_in_microseconds - old_time_in_microseconds;
    long highest_file_transfered_time = times[0];
    long shortest_file_transfered_time = times[0];

    for (int s=0; s < num_times; s++){
        if (times[s] > highest_file_transfered_time){
            highest_file_transfered_time = times[s];
        } else if(times[s] < highest_file_transfered_time){
            shortest_file_transfered_time = times[s];
        }
    }
    printf("---------------------------------------------------------\n");
    printf("                      General facts\n");
    printf("---------------------------------------------------------\n");
    printf("Total client files requested =             %d    (unit)\n", total_client_requests);
    printf("Successful files transfered =              %d   (%)\n", total_files_transfered);
    printf("Client request(s) time completion =        %lu (us)\n", total_time_request);
    printf("Highest file transfering period of time =  %lu (us)\n", highest_file_transfered_time);
    printf("Shortest file transfering period of time = %lu  (us)\n", shortest_file_transfered_time);
    printf("---------------------------------------------------------\n");
    printf("         Files transfered in microseconds\n");
    printf("---------------------------------------------------------\n");
    long total_time = 0;
    for (int s=0; s < num_times; s++){
        total_time += times[s];
        printf("File %d            %lu  (us)\n", (s+1), times[s]);
    }
    printf("---------------------------------------------------------\n");
    printf("         Files transfered per percentage\n");
    printf("---------------------------------------------------------\n");
    for (int s=0; s < num_times; s++){
        printf("File %d            %lu  (%)\n", (s+1), (times[s] * 100 / total_time));
    }


}

int main(int argc, char *argv[])
{
    receive_arguments(argc, argv);
    set_old_time();
    connect_to_server();
    set_new_time();
    if (file_output == 0) {
        server_statistics();
    }
    return 0;
}