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
#include <stdbool.h>
#include "common.h"
#include "socketLib/socket_lib.h"

int response;
int processLimit;

pid_t pid2;

bool stop = false;
static bool c_stop = false;

void child_handler();
void parent_handler();
void ChildExitSignalHandler();
void worker(int server_socket);

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
  close(c_socket_fd);
}

int main(int argc, char *argv[])
{
  pid_t processID;
  int processCt;
  struct sigaction myAction;

  if (argc != 2)
  {
    fprintf(stderr, "Usage:  %s <N-Forks>\n", argv[0]);
    exit(1);
  }
  processLimit = atoi(argv[1]);

  printf("INFO: Initializing PRE-FORK server.\n");

  int server_socket;

  if ((server_socket = slisten(PRE_FORK_PORT)) < 0)
  {
    error("PRE-FORK-Server: slisten");
  }

  /* Set ChildExitSignalHandler() as handler function */
  myAction.sa_handler = ChildExitSignalHandler;
  if (sigfillset(&myAction.sa_mask) < 0) /* mask all signals */
    printf("sigfillset() failed");
  /* SA_RESTART causes interrupted system calls to be restarted */
  myAction.sa_flags = SA_RESTART;

  /* Set signal disposition for child-termination signals */
  if (sigaction(SIGCHLD, &myAction, 0) < 0)
    printf("sigaction() failed");
  signal(SIGINT,parent_handler);
  printf("parent> %ld\n", getpid());
  for (processCt = 0; processCt < processLimit; processCt++)
  {
    /* Fork child process and report any errors */
    if ((processID = fork()) < 0)
    {
      perror("fork() failed");
      exit(1);
    }
    else if (processID == 0)
    {
      signal(SIGUSR1,child_handler);
      printf("child> %d\n",getpid());
      worker(server_socket);
      printf("child stopped\n");
      exit(0);
    }
    else
    {
      //pid2 = processID;
    }
  }

  while (!stop)
  {
    pause();
  }

  printf("parent Stopped\n");

  while (processLimit) /* Clean up all zombies */
    {
        processID = waitpid((pid_t) -1, NULL, WNOHANG);  /* Non-blocking wait */
        if (processID < 0)  /* waitpid() error? */
            printf("waitpid() failed");
        else if (processID == 0)  /* No zombie to wait on */
            break;
        else
            processLimit--;  /* Cleaned up after a child */
    }
  printf("parent TERM\n");

  return 0;
}

void worker(int server_socket)
{
  int client_socket;
  while (!c_stop) /* Run forever */
  {
    client_socket = saccept(server_socket);
    printf("with child process: %d\n", (unsigned int)getpid());
    handle_request(client_socket);
  }
  printf("worker() stopped\n");
}

void child_handler()
{
  c_stop = true;
  exit(1);
}

void parent_handler()
{
  kill(pid2, SIGUSR1);
  stop = true;
  c_stop = true;
}

void ChildExitSignalHandler()
{
  pid_t processID; /* Process ID from fork() */

  while (processID = waitpid(-1, NULL, WNOHANG)) {
    if (errno == ECHILD) {
        break;
    }
    printf("Cleaned up after a child %d\n", processID);
  }
}