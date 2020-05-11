#include <stdio.h>
#include <strings.h>
#include <arpa/inet.h>//for htonl and sockaddr_in
#include <unistd.h>//for getopt,read,write,close
#include <stdlib.h>//for atoi
#include <string.h>//for memset and memcpy
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>//for errno
#include <signal.h>//for signal func
#include <tpool.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"

#define TCP_PORT  7878
#define QUEUE_SIZE 10
#define POOL_SIZE 4

int response = 200;

void ctrl_c_signal_handler(int sig_num);
bool stop;
const int queue_size=15;

void handle_request(int c_socket_fd)
{
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

    while(1);
	close(file);
	free(aux_buffer);
	free(buffer);
  close(c_socket_fd);
}


int main(int argc, char *argv[]){
    int listenfd=0,connfd=0;
    int tcp_port=0, on=0;
    int num_threads=POOL_SIZE;

    struct sockaddr_in serv_addr;
    char opt;

    //AF_INET connection with differnent machine
    //SOCK_STREAM TCP connection
    //SOCK_NONBLOCK mark socket as non blocking
    //when accept is called and there is no pending connections
    //accept blocks but with SOCK_NONBLOCK it returns immidiately with 
    //error EAGAIN or EWOULDBLOCK
    if((listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0))== -1)
    {
        printf("Error : Could not create socket\n");
        printf("Errno %d\n",errno);
        return -1;
    }

    /* allow server to reuse address when binding */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
      perror("setsockopt SO_REUSEADDR");
    }
    /* allows UDP and TCP to reuse port (and address) when binding */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on)) < 0)
    {
      perror("setsockopt SO_REUSEPORT");
    }

    //optstring(last argument in getopt) is a string containing the legitimate option characters.
    //If such a character is followed by a colon, the option requires an
    //argument, so getopt() places a pointer to the following text in the
    //same argv-element, or the text of the following argv-element, in
    //optarg.  Two colons mean an option takes an optional arg
    while ((opt = getopt (argc, argv, "p:t:")) != -1){
        switch(opt){
            case 'p':
                tcp_port=9009;
                break;
            case 't':
                num_threads=2;
                break;
            case '?':
                //getopt gives ? if it is unable to recoganize an option
                printf("Error:wrong format\n");
                printf("Usage ./server [-t tcp_port]\n");
                return -1;
                break;
        }
    }

    if(tcp_port==0)
        tcp_port = TCP_PORT;
   

    memset(&serv_addr, '0', sizeof(serv_addr));

    //AF_INET tell that the connection is with different machine
    //AF_UNIX connect inside same machine
    serv_addr.sin_family = AF_INET;

    //To accept connctions from all IPs
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    //The htonl(uint32_t hostlong) function converts the unsigned integer 
    //hostlong from host byte order to network byte order.
    //htonl was giving wrong value htons gives correct
    serv_addr.sin_port=htons(tcp_port);

    if(bind(listenfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))==-1){
        printf("Error:Bindint with port # %d failed\n",tcp_port);
        printf("Errno %d\n",errno);
        if(errno == EADDRINUSE)
            printf("Another socket is already listening on the same port\n");
        return -1;
    }


    //The backlog(second arg of listen) argument defines the maximum length to which the queue of
    //pending connections for sockfd may grow. If a connection request 
    //arrives when the queue is full, the client may receive an error
    // with an indication of ECONNREFUSED or, if the
    // underlying protocol supports retransmission, 
    //the request may be ignored so that a later reattempt at connection succeeds.
    if(listen(listenfd, QUEUE_SIZE) == -1){
        printf("Error:Failed to listen\n");
        printf("Errno %d\n",errno);
        if(errno == EADDRINUSE)
            printf("Another socket is already listening on the same port\n");
        return -1;
    }

    printf("Lintning on TCP port %d\n",tcp_port);

    tpool_t *tp=tpool_create(num_threads);

    signal(SIGINT,ctrl_c_signal_handler);
    stop=false;
    
    while(!stop){
        connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request
        if(connfd!=-1){
            tpool_add_work(tp,handle_request,connfd);
        }else{
            //sleep for 0.5 seconds
            usleep(500000);
        }
    }
    
    close(listenfd);
    tpool_destroy(tp);//wait until all tasks are finished
    
    return 0;
}

void ctrl_c_signal_handler(int sig_num){
    printf("Exiting\n");
    stop=true;
}

