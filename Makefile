main : sproxy cproxy
	cp * ../10

sproxy : sproxy.c
	gcc -g -Wall -o sproxy sproxy.c

cproxy : cproxy.c
	gcc -g -Wall -o cproxy cproxy.c

.PHONY: clean
clean :
	rm -f *.o client server