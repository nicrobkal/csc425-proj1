/*
 * File to store functions/structs related to sockets moving between addresses
 */

#ifndef PORTABLE_SOCKET
#define PORTABLE_SOCKET
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

struct portableSocket {
	int socket;
	struct sockaddr_in address;
	int error;
};

struct portableSocket * createSocket(char * address, int port);

int portableBind(struct portableSocket * socket);

int portableListen(struct portableSocket * socket, int bufferSize);

struct portableSocket * portableAccept(struct portableSocket * socket);

int portableConnect(struct portableSocket * socket);

int portableSend(struct portableSocket * socket, char* message, int messageSize);

int portableRecv(struct portableSocket * socket, char* message, int bufferSize);

int portableClose(struct portableSocket* socket);

int portableCloseNetwork();

int portableCheckError(struct portableSocket * socket);
#endif
