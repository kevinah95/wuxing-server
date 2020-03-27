#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// FIFO server dedicated port
#define PORT 51717

// Fixed buffer size
#define BUFFER_SIZE 1024

// Handle the sockets' address
struct sockaddr_in server, client, cli_addr;

unsigned short port = PORT;

// Client and server file descriptors
int server_fd, client_socket;

socklen_t clilen;
char buffer[256];

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
	server.sin_port = htons(port);
    server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Port and socket are bind
	if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		error("ERROR: bind() failed. It was not possible to assign an address to the server.\n");
	}
	
	//se empieza a escuchar peticiones.
	if (listen(server_fd, 5) < 0) {
		error("ERROR: listen() failed. It is not possible to listen in the assigned port.\n");
	}

    while(1) {
        printf("Waiting to read messages...\n");

        clilen = sizeof(cli_addr);
        client_socket = accept(server_fd, 
                    (struct sockaddr *) &cli_addr, 
                    &clilen);
        
        if (client_socket < 0) {
            error("ERROR: Error on accept.\n");
        }

        bzero(buffer, 256);
        if (read(client_socket, buffer, 255) < 0) {
            error("ERROR: Error reading from socket.\n");
        }

        printf("Message from client: %s\n", buffer);
        if(write(client_socket, "Message from server: I got your message.", 41) < 0) {
            error("ERROR: Error writing to socket.\n");
        }
    }

    close(client_socket);
    close(server_fd);
}

int main(int argc, char *argv[]) {
    printf("INFO: Initializing server.\n");
    init();
    return 0;
}