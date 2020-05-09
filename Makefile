CC      := gcc -std=c11
CCFLAGS := -I.
LDFLAGS := -lpthread -lm -lrt

#SRC_DIRS ?= ./

TARGETS:= fifo_server fork_server thread_server pre_threaded_server client
MAINS  := $(addsuffix .o, $(TARGETS) )
OBJ    := $(MAINS)
#DEPS   :=

.PHONY: all clean

all: clean $(TARGETS)

clean:
	rm -f $(TARGETS) $(OBJ)

$(OBJ): %.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

$(TARGETS): % : $(filter-out $(MAINS), $(OBJ)) %.o
	$(CC) -o $@ $^ $(CCFLAGS) $(LDFLAGS)

test:
	gcc -c -o tpool.o tpool.c -I.
	gcc -c -o server.o server.c -I.
	gcc -o server tpool.o server.o -I. -lpthread -lm -lrt