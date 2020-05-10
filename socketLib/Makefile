CC=gcc
DOT_O = 
RANLIB = ranlib
ARFLAGS = ru
AR = ar
OBJS=saccept.o slisten.o
CFLAGS= -g
SRCS= socket_lib.h saccept.c slisten.c
HEADER= socket_lib.h

all: libsock.a

libsock.a: $(OBJS) $(HEADER)
	$(AR) $(ARFLAGS) libsock.a $(OBJS)
	$(RANLIB) libsock.a

saccept.o: saccept.c $(HEADER)
	$(CC) -c $(CFLAGS) -o saccept.o saccept.c

slisten.o: slisten.c $(HEADER)
	$(CC) -c $(CFLAGS) -o slisten.o slisten.c

clean:
	rm -f *.o *~
	rm -f libsock.a
