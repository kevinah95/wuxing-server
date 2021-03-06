#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define BUFFER_SIZE 1024 // Fixed buffer size

#define OK_HTTP 200             // Success response
const int HTTP_NOT_FOUND = 404; // File not found response

const char *FILE_NOT_FOUND = "file_not_found.html";

const char *SERVER_FILES = "./server_files/";

// HTTP response
const char *HTTP_RESPONSE = "HTTP/2.0 %d OK\r\nContent_type: Application/Octet-stream, text/html\r\n\n";

struct timespec oldTime;

struct timespec newTime;

struct timespec shortNewTime;

struct timespec shortOldTime;