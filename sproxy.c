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

struct portableSocket *clientAccept;
struct portableSocket *clientSocket;
struct portableSocket *telnetSocket;

int getNForSelect(int socket[], int numSockets)
{
    int max = -1;
    int i = 0;
    for (i = 0; i < numSockets; i++)
    {
        if (socket[i] > max)
        {
            max = socket[i];
        }
    }
    return max + 1;
}

struct portableSocket *getClient(struct portableSocket *clientAccept)
{
    struct portableSocket *client = portableAccept(clientAccept);
    if (portableCheckError(client) != 0)
    {
        fprintf(stderr, "Failed to create client socket \n");
        exit(1);
    }
    isClientConnected = 1;
    return client;
}

struct portableSocket *getTelnet()
{
    struct portableSocket *telnetSocket = createSocket("127.0.0.1", 23);
    portableConnect(telnetSocket);
    if (portableCheckError(telnetSocket) != 0)
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

int forward(struct portableSocket *sender, struct portableSocket *reciever, char *message, char *senderName)
{
    int messageSize = portableRecv(sender, message, size);
    if (portableCheckError(sender) != 0)
    {
        return -1;
    }
    if (isClientConnected == 1)
    {
        struct message messageStruct;
        createMessage(&messageStruct, MESSAGE, messageSize, message);
        sendMessageStruct(&messageStruct, reciever);
    }
    memset(message, 0, messageSize);
    return messageSize;
}

int sendMessage(struct portableSocket *reciever, char *message, int messageSize)
{
    portableSend(reciever, message, messageSize);
    memset(message, 0, size);
    return 0;
}

void sendHeartbeat(struct portableSocket *reciever)
{
    struct message messageStruct;
    char empty[0];
    empty[0] = '\0';
    createMessage(&messageStruct, HEARTBEAT, 0, empty);
    sendMessageStruct(&messageStruct, reciever);
}

int recvMessage(struct portableSocket *sender, struct portableSocket *reciever)
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
        portableClose(telnetSocket);
        telnetSocket = getTelnet();
        int socketN[] = {telnetSocket->socket, clientSocket->socket, clientAccept->socket};
        n = getNForSelect(socketN, 3);
        return 1;
    }
    return messageStruct.length;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return 1;
    }

    selectVal = 0;
    serverPort = atoi(argv[1]);
    
    clientAccept = createSocket("localhost", serverPort);
    if (portableCheckError(clientAccept) != 0)
    {
        fprintf(stderr, "Failed to connect to client accepter. Terminating.\n");
        return 1;
    }
    portableBind(clientAccept);
    portableListen(clientAccept, 5);

    clientSocket = portableAccept(clientAccept);
    if (portableCheckError(clientSocket) != 0)
    {
        fprintf(stderr, "Failed to connect to client. Terminating.\n");
        return 1;
    }
    isClientConnected = 1;

    struct message firstConnect;
    char empty[size];
    firstConnect.payload = empty;
    recvMessageStruct(&firstConnect, clientSocket);
    telnetSocket = getTelnet();

    fd_set readfds;
    int socketN[] = {clientSocket->socket, telnetSocket->socket, clientAccept->socket};
    n = getNForSelect(socketN, 3);
    char message[size];
    memset(message, 0, size);
    struct timeval tv = {3, 0};

    while (portableCheckError(clientSocket) == 0 && portableCheckError(telnetSocket) == 0)
    {
        reset(&readfds, telnetSocket->socket, clientSocket->socket, clientAccept->socket);
        struct timeval tv2 = {3, 0};
        struct timeval tv3 = {30, 0};
        selectVal = select(n, &readfds, NULL, NULL, &tv);
        if (FD_ISSET(telnetSocket->socket, &readfds))
        {
            int result = forward(telnetSocket, clientSocket, message, "telnet");
            if (result <= 0)
            {
                break;
            }
        }
        if (isClientConnected == 1 && FD_ISSET(clientSocket->socket, &readfds))
        {
            int result = recvMessage(clientSocket, telnetSocket);
            if (result <= 0)
            {
                break;
            }
            tv = tv2;
        }
        if (FD_ISSET(clientAccept->socket, &readfds))
        {
            clientSocket = getClient(clientAccept);
            isClientConnected = 1;
            int socketN[] = {telnetSocket->socket, clientSocket->socket, clientAccept->socket};
            n = getNForSelect(socketN, 3);
            struct message messageStruct;
            char message[size];
            messageStruct.payload = message;
            recvMessageStruct(&messageStruct, clientSocket);
            if (messageStruct.type == NEW_CONNECTION)
            {
                portableClose(telnetSocket);
                telnetSocket = getTelnet();
                int socketN[] = {telnetSocket->socket, clientSocket->socket};
                n = getNForSelect(socketN, 2);
            }
        }
        if (selectVal == 0 && isClientConnected == 0)
        {
            break;
        }
        if (selectVal == 0 && isClientConnected == 1)
        {
            portableClose(clientSocket);
            isClientConnected = 0;
            int socketN[] = {telnetSocket->socket, clientAccept->socket};
            n = getNForSelect(socketN, 2);
            tv = tv3;
        }
    }

    portableClose(clientAccept);
    portableClose(clientSocket);
    portableClose(telnetSocket);
    portableCloseNetwork();
    return 0;
}
