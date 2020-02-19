main : server client
	cp * ../10

server : server.c
	gcc -g -Wall -o server server.c

client : client.c
	gcc -g -Wall -o client client.c

.PHONY: clean
clean :
	rm -f *.o client server
