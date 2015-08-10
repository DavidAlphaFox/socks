PRGNAME = socks
OBJS = socks_buffer.o socks_pipe.o socks_queue.o\
 socks_address.o  socks_intercept.o socks_thread.o\
 socks_server.o socks_socket.o socks_task.o
 
CC=clang
# compile option to the executables
CFLAGS := -g -Wall -O3 -Iinc -I/usr/local/include
CXXFLAGS := $(CFLAGS)

LDFLAGS := -lpthread -lev -L/usr/local/lib

# vpath indicate the searching path of the according file type
SRCDIR := src $(shell ls -d src/*)
vpath %.c $(SRCDIR)
vpath %.h inc
vpath % bin

.PHONY: all $(PRGNAME)

all: $(PRGNAME)

clean :
	rm *.o
	cd bin;\
	rm -f $(PRGNAME);\
	cd ..;


$(PRGNAME):$(OBJS) 
	$(CC)  $(OBJS) $(LDFLAGS) -o $@
	mv $@ bin/


%.o: %.c
	$(CC) -c $(CFLAGS)  $<

