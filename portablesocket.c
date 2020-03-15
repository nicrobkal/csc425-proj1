/*
 * portablesocket.c
 */

#include "portablesocket.h"

//private functions
struct PortableSocket *newTCPSocket(char *address, int port);
int cpCheckError(struct PortableSocket *socket);

/*
 * newPortableSocket creates a new portable socket and returns the socket
 * for use. The first parameter protocol is either TCP or UDP in which the
 * portocol for that socket is set up. The second parameter address is a string
 * representation of the address the socket will connect to ex: "192.168.1.1".
 * The final parameter is an int which is the port number the socket will be open on.
 */
struct PortableSocket *cpSocket(int protocol, char *address, int port)
{
    if (protocol == 100)
        return newTCPSocket(address, port);
    return NULL;
}

/*
 * newTCPSocket is a private method that creates a new TCP socket. The first parameter
 * address is a string representation of the address the socket will connect
 * to ex: "192.168.1.1". The final parameter is an int which is the port number
 * the socket will be open on.
 */
struct PortableSocket *newTCPSocket(char *address, int port)
{
    //create the TCP socket
    int newsocket = socket(PF_INET, SOCK_STREAM, 0);

    //define server address
    struct sockaddr_in sock_address;
    memset(&sock_address, 0, sizeof(sock_address));
    sock_address.sin_family = AF_INET;
    sock_address.sin_port = htons(port);

    if (strcmp(address, "localhost") == 0)
    {
        sock_address.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        sock_address.sin_addr.s_addr = inet_addr(address);
    }

    //create the return type
    struct PortableSocket *newPS = malloc(sizeof(struct PortableSocket));
    newPS->socket = newsocket;
    newPS->address = sock_address;
    newPS->error = newsocket;
    newPS->error = cpCheckError(newPS);
    // returns the new socket
    return newPS;
}

/*
 * Binds the socket to the associated address space, allowing it to accept
 * connections from that address. The return value is 0 if the bind was successful
 * and -1 otherwise, the error value is updated accordingly.
 */
int cpBind(struct PortableSocket *socket)
{
    socket->error = bind(socket->socket, (struct sockaddr *)&socket->address, sizeof(socket->address));
    return cpCheckError(socket);
}

/*
 * The passiveListen function sets up the socket to passively listen for connections
 * to accept. The first parameter is the socket that will be set to passively listen
 * for connections, The second parameter is the size of the buffer, the number of connections
 * that can be queued at any given time.
 */
int cpListen(struct PortableSocket *socket, int bufferSize)
{
    socket->error = listen(socket->socket, bufferSize);
    return cpCheckError(socket);
}

/*
 * the acceptConnection function is a blocking call that will accept
 * a connection to the socket, this will create a new PortableSocket that
 * can then be used to communicate will the client socket.
 */
struct PortableSocket *cpAccept(struct PortableSocket *socket)
{
    int clientSocket = accept(socket->socket, NULL, NULL);
    struct PortableSocket *newPS = malloc(sizeof(struct PortableSocket));
    newPS->socket = clientSocket;
    newPS->address = socket->address;
    newPS->error = clientSocket;
    newPS->error = cpCheckError(newPS);
    return newPS;
}

/*
 * The connect function will have the socket connect to the address that it is associated
 * with. The connect function takes one parameter, which is the socket. The function returns
 * 0 if the connections was established, and returns an error code otherwise.
 */
int cpConnect(struct PortableSocket *socket)
{
    socket->error = connect(socket->socket, (struct sockaddr *)&socket->address, sizeof(socket->address));
    perror("connect");
    return cpCheckError(socket);
}

/*
 * The transmit function will transmit a message over the network through
 * the socket. The first parameter is the socket that will be transmiting the message.
 * The second parameter will be the message that is being transmitted.
 */
int cpSend(struct PortableSocket *socket, char *message, int messageSize)
{
    char *buffer = message;
    int length = messageSize;
    int i, flags = 0;
    while (messageSize > 0)
    {
        if (messageSize <= 0)
            break;
        i = send(socket->socket, buffer, length, flags);
        if (i != 0)
            break;
        buffer += i;
        length -= i;
    }

    socket->error = i;
    return cpCheckError(socket);
}

/*
 * The receive function will receive an incoming transmition. The first parameter is the
 * socket that will be receiving the message. The second parameter will be the
 * the variable the message being recieved will be stored. The third parameter is
 * The bufferSize of the message.
 */
int cpRecv(struct PortableSocket *socket, char *message, int bufferSize)
{
    socket->error = recv(socket->socket, message, bufferSize, 0);
    return (socket->error < 0) ? cpCheckError(socket) : socket->error;
}

/*
 * The closeSocket function will close the inputed socket, and free the memory used
 * by the socket. returns 0 if successful, error code otherwise.
 */
int cpClose(struct PortableSocket *socket)
{
    socket->error = close(socket->socket);
    int error = cpCheckError(socket);
    free(socket);
    return error;
}

/*
 * The closeNetwork function must be called whenever the application is finished using
 * all sockets and will no longer need acsess to the network. This function will clean
 * up any additional resources that may be used by the platform specific implementation.
 */
int cpCloseNetwork()
{
    return 0;
}

/*
 * checks to see if an error has occured and returns the error code, also sets
 * the Portable Sockets error field.
 */
int cpCheckError(struct PortableSocket *socket)
{
    if (socket->error < 0)
    {
        return socket->error;
    }
    else
    {
        return 0;
    }
}
