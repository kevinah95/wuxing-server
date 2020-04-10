#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

struct sockaddr_in server, client, cli_addr; // Handle the sockets' address

unsigned int client_lenght;

int server_fd, client_socket; // Client and server file descriptors

int response = OK_HTTP;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// Inits the server
void init() {
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		error("ERROR: It was impossible to open the Server.\n");
	}
	bzero(&server, sizeof(server));
	
	// Information required by the server
	server.sin_port = htons(FIFO_PORT);
    server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Port and socket are bind
	if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		error("ERROR: bind() failed. It was not possible to assign an address to the server.\n");
	}
	
	// Listen requests
	if (listen(server_fd, 100) < 0) {
		error("ERROR: listen() failed. It is not possible to listen in the assigned port.\n");
	}
}

void handle_request(int c_socket) {
    char *buffer = malloc(BUFFER_SIZE);
	char *aux_buffer = malloc(BUFFER_SIZE);
	int file;
	
    bzero(buffer, BUFFER_SIZE);

    if (recv(c_socket, buffer, BUFFER_SIZE, 0) <= 0){
		free(buffer);
        error("ERROR: It was not possible to receive message from client.\n");
	}

    // Gets file name
    char * file_name;
    file_name = strtok(buffer, " ");
	file_name = strtok(NULL, " ");
    
    char aux_path[100];
	strcpy(aux_path, "");
	strcat(aux_path, SERVER_FILES);
	strcat(aux_path, file_name);
	
    response = OK_HTTP;

	if ((file = open(aux_path, O_RDONLY, "r")) == -1){
		printf("The file %s does not exist.\nPlease note that the extension of the file is required.\n", aux_path);
		bzero(aux_path, sizeof(aux_path));
		strcpy(aux_path, SERVER_FILES);
		strcat(aux_path, FILE_NOT_FOUND);
		file = open(aux_path, O_RDONLY, "r");
		response = HTTP_NOT_FOUND;
	}

    char * header = malloc(BUFFER_SIZE);

    printf("Sending file: %s\n", aux_path);
	lseek(file, 0, SEEK_SET);
	bzero(header, BUFFER_SIZE);
	
	sprintf(header, HTTP_RESPONSE, response);
	
	if (send(c_socket, header, strlen(header), 0) == -1){
		printf("ERROR: Data sending failed.\n");
		free(header);
		close(file);
		return;
	}
	free(header);

	bzero(aux_buffer, BUFFER_SIZE);
    int chunk_length;
	while((chunk_length = read(file, aux_buffer, BUFFER_SIZE)) > 0) {
		if (send(c_socket, aux_buffer, chunk_length, 0) == -1) {
			printf("ERROR: Something went wrong while sending the file.\n");
			return;
		}
	}
	
	printf("File successfully sent.\n");
	close(file);
	free(aux_buffer);
	free(buffer);
}

void listen_requests() {
	while(1) {
		client_lenght = sizeof(client);
		
		if ((client_socket = accept(server_fd, (struct sockaddr *)&client, &client_lenght)) < 0) {
			printf("ERROR: It was not possible to accept the request.\n");
		}
		else {
			printf("Attending client: %s\n", inet_ntoa(client.sin_addr));
			handle_request(client_socket);
			close(client_socket);
		}
	}
}

int main(int argc, char *argv[]) {
    printf("INFO: Initializing server.\n");
    init();
    listen_requests();
    return 0;
}