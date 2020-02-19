main:
	gcc -g -Wall -o server server.c
	gcc -g -Wall -o client client.c

.PHONY: clean

clean:
	rm -f *.o
