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
#include "socketLib/socket_lib.h"

int response;
int processLimit;
int alarmFlag = 0;

int childflag = 0;

void ChildExitSignalHandler();
static void child_handler(void);

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
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = child_handler;

  sigaction(SIGCHLD, &sa, NULL);
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

  //signal(SIGINT,ChildExitSignalHandler);

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
      ProcessMain(server_socket);
      exit(0);
    }
  }

  pid_t pid;
  int status;
  /* while (processLimit > 0)
  {
    pid = wait(&status);
    printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
    --processLimit;
  } */
  while (!alarmFlag)
  {
    printf ("Enter loop %d\n", processLimit);
    pause(); /* Wait for a signal */
  }
  
    printf ("Loop ends due to alarm signal\n");
    

  return 0;
}

void ProcessMain(int server_socket)
{
  int client_socket;
  signal(SIGINT,ChildExitSignalHandler);
  while (!childflag) /* Run forever */
  {
    client_socket = saccept(server_socket);
    printf("with child process: %d\n", (unsigned int)getpid());
    handle_request(client_socket);
  }

}

static void
child_handler(void)
{
  childflag = 1;
  printf(">>>childflag");
    pid_t pid;
    int status;

    /* EEEEXTEERMINAAATE! */
    while((pid = waitpid(-1, &status, WNOHANG)) > 0)
        ;
}



void ChildExitSignalHandler()
{
  pid_t processID; /* Process ID from fork() */
  alarmFlag = 1;
  printf("---Child with PID %ld exited.processLimit %d\n", (long)processID, processLimit);
  while (processLimit) /* Clean up all zombies */
  {
    processID = waitpid((pid_t)-1, NULL, WNOHANG); /* Non-blocking wait */
    printf("Child with PID %ld exited.processLimit %d\n", (long)processID, processLimit);
    if (processID < 0){ /* waitpid() error? */
      printf("waitpid() failed");
      break;
    }
    else if (processID == 0){ /* No child to wait on */
      break;
    }
    else
      processLimit--; /* Cleaned up after a child */
  }
}