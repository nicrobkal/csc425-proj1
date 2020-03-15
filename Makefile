.PHONY: all clean copy

all: sproxy cproxy copy

copy:
	cp * ../10

%.o: %.c message.h portablesocket.h
	gcc -Wall -g -c -o $@ $<

cproxy: cproxy.o portablesocket.o message.o
	gcc -Wall -g -o $@ $^

sproxy: sproxy.o portablesocket.o message.o
	gcc -Wall -g -o $@ $^

clean:
	rm *.o sproxy cproxy
