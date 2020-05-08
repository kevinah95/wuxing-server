/*
    C socket server example, handles multiple clients using threads
    Compile
    gcc server.c -lpthread -o server
*/
 
#include<stdio.h>
#include<string.h>    //strlen
#include <strings.h>
#include<stdlib.h>    //strlen
#include <sys/file.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "common.h"
 
int response = 200;
//the thread function
void *connection_handler(void *);

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

	/* Re-instate handler */
	signal(SIGCHLD, sigchld_handler);
}
 
int main(int argc , char *argv[])
{

    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;

    signal(SIGCHLD, sigchld_handler);
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *c_socket_fd)
{
    //Get the socket descriptor
    char *buffer = malloc(BUFFER_SIZE);
	char *aux_buffer = malloc(BUFFER_SIZE);
	int file;

	bzero(buffer, BUFFER_SIZE);

	if (recv(c_socket_fd, buffer, BUFFER_SIZE, 0) < 0)
	{
		free(buffer);
		printf("--------");
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
         
    return 0;
} 