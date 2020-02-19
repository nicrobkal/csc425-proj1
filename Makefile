main:
	gcc -g -Wall -o server server.c
	gcc -g -Wall -o client client.c
	cp * ../10

.PHONY: clean

clean:
	rm -f *.o
