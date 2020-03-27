#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <pthread.h>                                          
#include <sys/timeb.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024 // Fixed buffer size

#define PORT 51717 // FIFO server dedicated port

struct sockaddr_in server; // Handle the socket address

int server_fd; // Server file descriptor

char * ip; // Server IP address

char * requested_file; // File requested by client

pthread_t thread; // pthread that's going to be created to perform client requests
unsigned int threads = 0; // Number of threads that will be executed
unsigned int cycles = 0; // Number of cycles a thread will repeat a request
unsigned short port = PORT; // Server's port

// Contain info required by the thread: server_socket to connect, and message to send
struct thread_info_st {
	int server_socket;
	char* message;
};

char buffer[1024] = "Hi from the client!";
char buffer222[256] = "Hi from the client!";

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void receive_arguments(int argc, char* argv[]) {
	if (argc != 6) {
		error("ERROR: Wrong number/format of the parameters.\nThe expected format is: ./client <ip> <port> <file> <N-threads> <N-cycles>");
	}
	ip = argv[1];
	port = atoi(argv[2]);
	requested_file = argv[3];
	threads = atoi(argv[4]);
	cycles = atoi(argv[5]);
}

void * get(void* threads_st) {
	struct thread_info_st * thread = (struct thread_info_st *)threads_st;
	cycles = 5;

	for(int i=0; i<cycles; i++) {
		struct thread_info_st cycle_thread = thread[i];
		int cycle_socket = cycle_thread.server_socket;
		char * cycle_message = cycle_thread.message;
		printf("DEBUG: Trying to write message.\n");

		printf("DEBUG: Message: %s\n", cycle_message);

		if (write(cycle_socket, cycle_message, strlen(cycle_message)) < 0) {
			error("ERROR: Error writing to server socket");
		}

		bzero(buffer, 1024);
		if (read(cycle_socket, buffer, 1023) < 0) {
			error("ERROR: Error reading from socket");
		}
		printf("%s\n", buffer);
	}
}

// Create the connection with the server
void connect_to_server() {
	int index = 0;
	threads = 1;
	cycles = 5;

	struct thread_info_st thread_data[cycles];
	
	do {
		for(int i=0; i<cycles; i++) {
			printf("DEBUG: Getting server socket, cycle %i...\n", i);
			if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
				error("ERROR: It was not possible to establish connection with the socket.\n");
			}

			bzero(&server, sizeof(server));
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = inet_addr("127.0.0.1");
			server.sin_port = htons(port);

			printf("DEBUG: Connecting to server, cycle %i...\n", i);

			if (connect(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0){
				error("ERROR: Connection with server failure.\n");
			}
			thread_data[i].server_socket = server_fd;
			thread_data[i].message = buffer222;
		}
		printf("DEBUG: Creating pthread, thread %i...\n", index);
		if(pthread_create(&thread, NULL, get, (void *)thread_data) != 0) {
			error("ERROR: It was not possible to create a pthread.\n");
		}
		pthread_join(thread, NULL);

		index++;
	} while(index < threads);

    close(server_fd);
}

int main(int argc, char *argv[]) {
	//receive_arguments(argc, argv);
    connect_to_server();
    return 0;
}