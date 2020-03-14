/*
 * PortableSocket.h
 * A socket interface compatiable on Windows and Ubuntu
 *
 *  Created on: Jan 23, 2017
 *      Author: Jonathon Davis
 */

#ifndef PORTABLE_SOCKET
#define PORTABLE_SOCKET
//loads the two seperate socket libraries
#ifdef __WIN32__
# include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
//loads common libraries that will be needed
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

#define TCP 100
#define UDP 200

/*
 * openNetwork is the first function that must be called
 * whenever sockets are going to be used. This function initializes
 * certain parameters that are system dependent. This function only ever
 * needs to be called once, and will allow the creation and use of sockets
 * throughout the remainder of the programs runtime.
 */
int cpOpenNetwork();

/*
 * newPortableSocket creates a new portable socket and returns the socket
 * for use. The first parameter protocol is either TCP or UDP in which the
 * portocol for that socket is set up. The second parameter address is a string
 * representation of the address the socket will connect to ex: "192.168.1.1".
 * The final parameter is an int which is the port number the socket will be open on.
 */
struct PortableSocket * cpSocket(int protocol, char * address, int port);

/*
 * Binds the socket to the associated address space, allowing it to accept
 * connections from that address. The return value is 0 if the bind was successful
 * and -1 otherwise, the error value is updated accordingly.
 */
int cpBind(struct PortableSocket * socket);

/*
 * The passiveListen function sets up the socket to passively listen for connections
 * to accept. The first parameter is the socket that will be set to passively listen
 * for connections, The second parameter is the size of the buffer, the number of connections
 * that can be queued at any given time.
 */
int cpListen(struct PortableSocket * socket, int bufferSize);

/*
 * the acceptConnection function is a blocking call that will accept
 * a connection to the socket, this will create a new PortableSocket that
 * can then be used to communicate will the client socket.
 */
struct PortableSocket * cpAccept(struct PortableSocket * socket);

/*
 * The connect function will have the socket connect to the address that it is associated
 * with. The connect function takes one parameter, which is the socket. The function returns
 * 0 if the connections was established, and returns an error code otherwise.
 */
int cpConnect(struct PortableSocket * socket);

/*
 * The transmit function will transmit a message over the network through
 * the socket. The first parameter is the socket that will be transmiting the message.
 * The second parameter will be the message that is being transmitted.
 */
int cpSend(struct PortableSocket * socket, char* message, int messageSize);

/*
 * The receive function will receive an incoming transmition. The first parameter is the
 * socket that will be receiving the message. The second parameter will be the
 * the variable the message being received will be stored. The third parameter is
 * The bufferSize of the message.
 */
int cpRecv(struct PortableSocket * socket, char* message, int bufferSize);

/*
 * The closeSocket function will close the inputed socket, and free the memory used
 * by the socket.
 */
int cpClose(struct PortableSocket* socket);

/*
 * The closeNetwork function must be called whenever the application is finished using
 * all sockets and will no longer need access to the network. This function will clean
 * up any additional resources that may be used by the platform specific implementation.
 */
int cpCloseNetwork();

/*
 * checks to see if an error has occured and returns the error code, also sets
 * the Portable Sockets error field.
 */
int cpCheckError(struct PortableSocket * socket);
#endif
