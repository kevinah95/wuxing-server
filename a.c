#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#define WORKER_POOL_SIZE 2

void worker ( int command_fd) {
  printf("childpid %ld, parent pid = %ld\n", (long)getpid(), (long)getppid());
  return;
}

int
main(int argc, char *argv[])
{
    pid_t workers[WORKER_POOL_SIZE];
    pid_t cpid, w;
    int status;
    int n = WORKER_POOL_SIZE;

    for (int i = 0 ; i < WORKER_POOL_SIZE ; i++ ) {
        workers[i] = fork();
        if ( workers[i] == 0 ) {
            worker( 1 );
            pause(); 
            exit(0);  // should never get here
        } 
    }

    /* Wait for children to exit. */
    pid_t pid;
    while (n > 0) {
      pid = wait(&status);
      printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
      --n;  // TODO(pts): Remove pid from the pids array.
    }

  //  cpid = fork();
  //   if (cpid == -1) {
  //       perror("fork");
  //       exit(EXIT_FAILURE);
  //   }

  //  if (cpid == 0) {            /* Code executed by child */
  //       printf("childpid %ld, parent pid = %ld\n", (long)getpid(), (long)getppid());
  //       if (argc == 1)
  //           pause();                    /* Wait for signals */
  //       _exit(atoi(argv[1]));

  //  } else {                    /* Code executed by parent */
  //       do {
  //           w = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
  //           if (w == -1) {
  //               perror("waitpid");
  //               exit(EXIT_FAILURE);
  //           }

  //          if (WIFEXITED(status)) {
  //               printf("exited, status=%d\n", WEXITSTATUS(status));
  //           } else if (WIFSIGNALED(status)) {
  //               printf("killed by signal %d\n", WTERMSIG(status));
  //           } else if (WIFSTOPPED(status)) {
  //               printf("stopped by signal %d\n", WSTOPSIG(status));
  //           } else if (WIFCONTINUED(status)) {
  //               printf("continued\n");
  //           }
  //       } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  //       exit(EXIT_SUCCESS);
  //   }
}