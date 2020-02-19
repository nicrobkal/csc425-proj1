main : server client sproxy cproxy
	cp * ../10

server : server.c
	gcc -g -Wall -o server server.c

client : client.c
	gcc -g -Wall -o client client.c

sproxy : sproxy.c
	gcc -g -Wall -o sproxy sproxy.c

cproxy : cproxy.c
	gcc -g -Wall -o cproxy cproxy.c

.PHONY: clean
clean :
	rm -f *.o client server
