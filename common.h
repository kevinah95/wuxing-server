#include <sys/types.h>

#define BUFFER_SIZE 1024 // Fixed buffer size

// Servers ports
#define FIFO_PORT 51717
#define THREAD_PORT 51718
#define PRE_THREAD_PORT 51719

#define OK_HTTP 200             // Success response
const int HTTP_NOT_FOUND = 404; // File not found response

const char *FILE_NOT_FOUND = "file_not_found.html";

const char *SERVER_FILES = "./server_files/";

// HTTP response
const char *HTTP_RESPONSE = "HTTP/2.0 %d OK\r\nContent_type: Application/Octet-stream, text/html\r\n\n";