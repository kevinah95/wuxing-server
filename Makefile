CC=gcc -g
AR = ar
ARFLAGS = ru
RANLIB = ranlib
CFLAGS= -g
SRCS= client.c fifo_server.c fork_server.c pre_threaded_server.c thread_server.c 
LIBS = -L./socketLib/

all:: socketlib fifo_server fork_server pre_threaded_server thread_server client pre_fork_server

socketlib:
	cd socketLib && make

fifo_server: fifo_server.o
	$(CC) -o fifo_server fifo_server.o $(LIBS) -lsock -lpthread

fork_server: fork_server.o
	$(CC) -o fork_server fork_server.o $(LIBS) -lsock -lpthread

pre_fork_server: pre_fork_server.o
	$(CC) -o pre_fork_server pre_fork_server.o $(LIBS) -lsock -lpthread

pre_threaded_server: pre_threaded_server.o
	$(CC) -o pre_threaded_server pre_threaded_server.o $(LIBS) -lsock -lpthread -lrt

thread_server: thread_server.o
	$(CC) -o thread_server thread_server.o $(LIBS) -lsock -lpthread

client: client.o
	$(CC) -o client client.o -lpthread

fifo_server.o: fifo_server.c
	$(CC) -o fifo_server.o -c fifo_server.c

fork_server.o: fork_server.c
	$(CC) -o fork_server.o -c fork_server.c

pre_threaded_server.o: pre_threaded_server.c
	$(CC) -o pre_threaded_server.o -c pre_threaded_server.c

thread_server.o: thread_server.c
	$(CC) -o thread_server.o -c thread_server.c

clean:
	/bin/rm -f client fifo_server fork_server pre_threaded_server thread_server pre_fork_server *.o core *~ #*
	cd socketLib && make clean
