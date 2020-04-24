CC      := gcc -std=c11
CCFLAGS := 
LDFLAGS := -lpthread -lm -lrt

#SRC_DIRS ?= ./

TARGETS:= fifo_server fork_server client
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
	$(CC) -o $@ $(LIBS) $^ $(CCFLAGS) $(LDFLAGS)
