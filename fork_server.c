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

#define PORT 4444

int response = 200;

void error(const char *msg)
{
	perror(msg);
	exit(1);
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

	/* Re-instate handler */
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
}

int main()
{

	int sockfd, ret;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	char buffer[1024];
	pid_t childpid;

	/*
		* Set signal handler for SIGCHLD :
		*/
	//signal(SIGCHLD, sigchld_handler);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int option_yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option_yes, sizeof(option_yes));

	if (sockfd < 0)
	{
		printf("[-]Error in connection:\n\t%s", strerror(errno));
		exit(1);
	}
	printf("[+]Server Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret < 0)
	{
		printf("[-]Error in binding:\n\t%s", strerror(errno));
		exit(1);
	}

	printf("[+]Bind to port %d\n", PORT);
	ret = listen(sockfd, 100) == 0;
	if (ret)
	{
		printf("[+]Listening....\n");
	}
	else
	{
		printf("[-]Error in binding:\n\t%s", strerror(errno));
	}

	while (1)
	{
		newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
		if (newSocket < 0)
		{
			exit(1);
		}
		printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
		childpid = fork();

		printf("childpid=%d", childpid);
		// Error
		if (childpid < 0)
		{
			close(newSocket);
			continue;
		}
		// Child
		else if (childpid == 0)
		{
			close(sockfd);
			while (1)
			{

				handle_request(newSocket);
				close(newSocket);
			}
		}
		// Parent
		else
		{
			close(newSocket);
			printf("here2\n");
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
