/*
 * File to store functions/structs related to sockets moving between addresses
 */

#include "portablesocket.h"

int portableCheckError(struct PortableSocket *socket);

struct PortableSocket *createSocket(char *address, int port)
{
    int newSocket = socket(PF_INET, SOCK_STREAM, 0);

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

    struct PortableSocket *newPS = malloc(sizeof(struct PortableSocket));
    newPS->socket = newSocket;
    newPS->address = sock_address;
    newPS->error = newSocket;
    newPS->error = portableCheckError(newPS);
    return newPS;
}

int portableBind(struct PortableSocket *socket)
{
    socket->error = bind(socket->socket, (struct sockaddr *)&socket->address, sizeof(socket->address));
    return portableCheckError(socket);
}

int portableListen(struct PortableSocket *socket, int bufferSize)
{
    socket->error = listen(socket->socket, bufferSize);
    return portableCheckError(socket);
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
    newPS->error = portableCheckError(newPS);
    return newPS;
}

/*
 * The connect function will have the socket connect to the address that it is associated
 * with. The connect function takes one parameter, which is the socket. The function returns
 * 0 if the connections was established, and returns an error code otherwise.
 */
int portableConnect(struct PortableSocket *socket)
{
    socket->error = connect(socket->socket, (struct sockaddr *)&socket->address, sizeof(socket->address));
    return portableCheckError(socket);
}

/*
 * The transmit function will transmit a message over the network through
 * the socket. The first parameter is the socket that will be transmiting the message.
 * The second parameter will be the message that is being transmitted.
 */
int portableSend(struct PortableSocket *socket, char *message, int messageSize)
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
    return portableCheckError(socket);
}

/*
 * The receive function will receive an incoming transmition. The first parameter is the
 * socket that will be receiving the message. The second parameter will be the
 * the variable the message being recieved will be stored. The third parameter is
 * The bufferSize of the message.
 */
int portableRecv(struct PortableSocket *socket, char *message, int bufferSize)
{
    socket->error = recv(socket->socket, message, bufferSize, 0);
    return (socket->error < 0) ? portableCheckError(socket) : socket->error;
}

/*
 * The closeSocket function will close the inputed socket, and free the memory used
 * by the socket. returns 0 if successful, error code otherwise.
 */
int portableClose(struct PortableSocket *socket)
{
    socket->error = close(socket->socket);
    int error = portableCheckError(socket);
    free(socket);
    return error;
}

/*
 * The closeNetwork function must be called whenever the application is finished using
 * all sockets and will no longer need acsess to the network. This function will clean
 * up any additional resources that may be used by the platform specific implementation.
 */
int portableCloseNetwork()
{
    return 0;
}

/*
 * checks to see if an error has occured and returns the error code, also sets
 * the Portable Sockets error field.
 */
int portableCheckError(struct PortableSocket *socket)
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
