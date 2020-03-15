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

struct PortableSocket {
	int socket;
	struct sockaddr_in address;
	int error;
};

struct PortableSocket * createSocket(char * address, int port);

int portableBind(struct PortableSocket * socket);

int portableListen(struct PortableSocket * socket, int bufferSize);

struct PortableSocket * portableAccept(struct PortableSocket * socket);

int portableConnect(struct PortableSocket * socket);

int portableSend(struct PortableSocket * socket, char* message, int messageSize);

int portableRecv(struct PortableSocket * socket, char* message, int bufferSize);

int portableClose(struct PortableSocket* socket);

int portableCloseNetwork();

int portableCheckError(struct PortableSocket * socket);
#endif
