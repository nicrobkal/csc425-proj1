CC = gcc
CC_FLAGS = -Wall -g
DEPS = message.h portablesocket.h

.PHONY: all clean

all: sproxy cproxy

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

cproxy: cproxy.o portablesocket.o message.o
	$(CC) $(CFLAGS) -o $@ $^

sproxy: sproxy.o portablesocket.o message.o
	$(CC) $(CFLAGS) -o $@ $^

#clean the object files and executables
clean:
	rm *.o sproxy cproxy
