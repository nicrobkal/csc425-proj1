/*
 * Server-side program to create seamless connection for a telnet session intending to move between WiFi networks
 */

#include "portablesocket.h"
#include <sys/select.h>
#include "message.h"

int selectVal = 0;
int serverPort = 0;
int isClientConnected = 0;
int n = 0;
int size = 1024;

struct PortableSocket *clientAcceptor;
struct PortableSocket *clientProxy;
struct PortableSocket *telnetSocket;

int getN(int socket[], int numberOfSockets)
{
    int max = -1;
    int i = 0;
    for (i = 0; i < numberOfSockets; i++)
    {
        if (socket[i] > max)
        {
            max = socket[i];
        }
    }
    return max + 1;
}

struct PortableSocket *getClientAcceptor(int serverPort)
{
    struct PortableSocket *clientAcceptor = cpSocket(100, "localhost", serverPort);
    if (cpCheckError(clientAcceptor) != 0)
    {
        fprintf(stderr, "Failed to create client acceptor socket \n");
        exit(1);
    }
    cpBind(clientAcceptor);
    cpListen(clientAcceptor, 5);
    return clientAcceptor;
}

struct PortableSocket *getClient(struct PortableSocket *clientAcceptor)
{
    struct PortableSocket *client = cpAccept(clientAcceptor);
    if (cpCheckError(client) != 0)
    {
        fprintf(stderr, "Failed to create client socket \n");
        exit(1);
    }
    isClientConnected = 1;
    return client;
}

struct PortableSocket *getTelnet()
{
    struct PortableSocket *telnetSocket = cpSocket(100, "127.0.0.1", 23);
    cpConnect(telnetSocket);
    if (cpCheckError(telnetSocket) != 0)
    {
        fprintf(stderr, "Failed to create telnet acceptor socket \n");
        exit(1);
    }
    return telnetSocket;
}

void reset(fd_set *readfds, int telnetSocket, int clientSocket, int clientAcceptor)
{
    FD_CLR(telnetSocket, readfds);
    if (isClientConnected == 1)
    {
        FD_CLR(clientSocket, readfds);
    }
    FD_CLR(clientAcceptor, readfds);
    FD_ZERO(readfds);
    if (isClientConnected == 1)
    {
        FD_SET(clientSocket, readfds);
    }
    FD_SET(clientAcceptor, readfds);
    FD_SET(telnetSocket, readfds);
}

int forward(struct PortableSocket *sender, struct PortableSocket *reciever, char *message, char *senderName)
{
    int messageSize = cpRecv(sender, message, size);
    if (cpCheckError(sender) != 0)
    {
        return -1;
    }
    if (isClientConnected == 1)
    {
        struct message messageStruct;
        initMessageStruct(&messageStruct, MESSAGE, messageSize, message);
        sendMessageStruct(&messageStruct, reciever);
    }
    memset(message, 0, messageSize);
    return messageSize;
}

int sendMessage(struct PortableSocket *reciever, char *message, int messageSize)
{
    cpSend(reciever, message, messageSize);
    memset(message, 0, size);
    return 0;
}

void sendHeartbeat(struct PortableSocket *reciever)
{
    struct message messageStruct;
    char empty[0];
    empty[0] = '\0';
    initMessageStruct(&messageStruct, HEARTBEAT, 0, empty);
    sendMessageStruct(&messageStruct, reciever);
}

int recvMessage(struct PortableSocket *sender, struct PortableSocket *reciever)
{
    struct message messageStruct;
    char message[size];
    messageStruct.payload = message;
    recvMessageStruct(&messageStruct, sender);
    if (messageStruct.type == MESSAGE)
    {
        sendMessage(reciever, messageStruct.payload, messageStruct.length);
    }
    else if (messageStruct.type == HEARTBEAT)
    {
        sendHeartbeat(sender);
        return 1;
    }
    else if (messageStruct.type == NEW_CONNECTION)
    {
        printf("Creating new telnet session\n");
        cpClose(telnetSocket);
        telnetSocket = getTelnet();
        int socketN[] = {telnetSocket->socket, clientProxy->socket, clientAcceptor->socket};
        n = getN(socketN, 3);
        return 1;
    }
    return messageStruct.length;
}

void parseInput(int argc, char *argv[])
{
    int current = 1;
    selectVal = 0;
    serverPort = atoi(argv[current++]);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return 1;
    }
        
    parseInput(argc, argv);

    clientAcceptor = getClientAcceptor(serverPort);
    clientProxy = getClient(clientAcceptor);

    struct message firstConnect;
    char empty[size];
    firstConnect.payload = empty;
    recvMessageStruct(&firstConnect, clientProxy);
    telnetSocket = getTelnet();

    fd_set readfds;
    int socketN[] = {clientProxy->socket, telnetSocket->socket, clientAcceptor->socket};
    n = getN(socketN, 3);
    char message[size];
    memset(message, 0, size);
    struct timeval tv = {3, 0};

    while (cpCheckError(clientProxy) == 0 && cpCheckError(telnetSocket) == 0)
    {
        reset(&readfds, telnetSocket->socket, clientProxy->socket, clientAcceptor->socket);
        struct timeval tv2 = {3, 0};
        struct timeval tv3 = {30, 0};
        selectVal = select(n, &readfds, NULL, NULL, &tv);
        if (FD_ISSET(telnetSocket->socket, &readfds))
        {
            int result = forward(telnetSocket, clientProxy, message, "telnet");
            if (result <= 0)
            {
                break;
            }
        }
        if (isClientConnected == 1 && FD_ISSET(clientProxy->socket, &readfds))
        {
            int result = recvMessage(clientProxy, telnetSocket);
            if (result <= 0)
            {
                break;
            }
            tv = tv2;
        }
        if (FD_ISSET(clientAcceptor->socket, &readfds))
        {
            clientProxy = getClient(clientAcceptor);
            isClientConnected = 1;
            int socketN[] = {telnetSocket->socket, clientProxy->socket, clientAcceptor->socket};
            n = getN(socketN, 3);
            struct message messageStruct;
            char message[size];
            messageStruct.payload = message;
            recvMessageStruct(&messageStruct, clientProxy);
            if (messageStruct.type == NEW_CONNECTION)
            {
                cpClose(telnetSocket);
                telnetSocket = getTelnet();
                int socketN[] = {telnetSocket->socket, clientProxy->socket};
                n = getN(socketN, 2);
            }
        }
        if (selectVal == 0 && isClientConnected == 0)
        {
            break;
        }
        if (selectVal == 0 && isClientConnected == 1)
        {
            cpClose(clientProxy);
            isClientConnected = 0;
            int socketN[] = {telnetSocket->socket, clientAcceptor->socket};
            n = getN(socketN, 2);
            tv = tv3;
        }
    }

    cpClose(clientAcceptor);
    cpClose(clientProxy);
    cpClose(telnetSocket);
    cpCloseNetwork();
    return 0;
}
