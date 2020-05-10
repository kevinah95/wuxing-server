extern int saccept (int socket_server_fd);
extern int slisten (int port);

// Servers ports
#define FIFO_PORT 51717
#define FORK_PORT 51718
#define THREAD_PORT 51719
#define PRE_THREAD_PORT 51720
#define PRE_FORK_PORT 51721

#define QUEUE_LISTENING_SIZE 100
