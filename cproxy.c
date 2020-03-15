/*
 * Client-side program to create seamless connection for a telnet session intending to move between WiFi networks
 */

#include "portablesocket.h"
#include <sys/select.h>
#include "message.h"

int selectVal;
char *serverAddr;
int clientPort, serverPort;
int lostHeartbeats;
struct PortableSocket *telnetAcceptorSocket;
struct PortableSocket *telnetSocket;
struct PortableSocket *sproxySocket;
int n;
int size = 1024;

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

void parseInput(int argc, char *argv[])
{
    int curr = 1;
    selectVal = 0;
    clientPort = atoi(argv[curr++]);
    serverAddr = argv[curr++];
    serverPort = atoi(argv[curr++]);
}

struct PortableSocket *getTelnetAcceptor()
{
    struct PortableSocket *telnetAcceptorSocket = createSocket("127.0.0.1", clientPort);
    if (portableCheckError(telnetAcceptorSocket) != 0)
    {
        fprintf(stderr, "Failed to create telnet acceptor socket\n");
        exit(1);
    }
    portableBind(telnetAcceptorSocket);
    portableListen(telnetAcceptorSocket, 5);
    return telnetAcceptorSocket;
}

struct PortableSocket *getTelnet(struct PortableSocket *telnetAcceptorSocket)
{
    struct PortableSocket *telnetSocket = portableAccept(telnetAcceptorSocket);
    if (portableCheckError(telnetSocket) != 0)
    {
        fprintf(stderr, "Failed to create telnet socket \n");
        exit(1);
    }
    return telnetSocket;
}

struct PortableSocket *getSproxy()
{
    struct PortableSocket *sproxySocket = createSocket(serverAddr, serverPort);
    portableConnect(sproxySocket);
    if (portableCheckError(sproxySocket) != 0)
    {
        fprintf(stderr, "Failed to create sproxy socket\n");
        exit(1);
    }
    return sproxySocket;
}

void reset(fd_set *readfds, int telnetSocket, int serverSocket)
{
    FD_CLR(telnetSocket, readfds);
    FD_CLR(serverSocket, readfds);
    FD_CLR(telnetAcceptorSocket->socket, readfds);
    FD_ZERO(readfds);
    FD_SET(serverSocket, readfds);
    FD_SET(telnetSocket, readfds);
    FD_SET(telnetAcceptorSocket->socket, readfds);
}

int forward(struct PortableSocket *sender, struct PortableSocket *reciever, char *message, char *senderName)
{
    int messageSize = portableRecv(sender, message, size);
    if (portableCheckError(sender) != 0)
    {
        return -1;
    } 
    struct message messageStruct;
    createMessage(&messageStruct, MESSAGE, messageSize, message);
    sendMessageStruct(&messageStruct, reciever);
    return messageSize;
}

int sendMessage(struct PortableSocket *reciever, char *message, int messageSize)
{
    portableSend(reciever, message, messageSize);
    memset(message, 0, messageSize);
    return 0;
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
        printf("recived heartbeat reply\n");
        lostHeartbeats = 0;
        return 1;
    }
    return messageStruct.length;
}

void sendHeartbeat(struct PortableSocket *reciever)
{
    lostHeartbeats++;
    struct message messageStruct;
    char empty[0];
    empty[0] = '\0';
    createMessage(&messageStruct, HEARTBEAT, 0, empty);
    sendMessageStruct(&messageStruct, reciever);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        return 1;
    }

    parseInput(argc, argv);

    telnetAcceptorSocket = getTelnetAcceptor();
    telnetSocket = getTelnet(telnetAcceptorSocket);

    sproxySocket = getSproxy();
    struct message newConnectStruct;
    char empty[0];
    empty[0] = '\0';
    createMessage(&newConnectStruct, NEW_CONNECTION, 0, empty);
    sendMessageStruct(&newConnectStruct, sproxySocket);

    fd_set readfds;
    int socketN[] = {sproxySocket->socket, telnetSocket->socket, telnetAcceptorSocket->socket};
    n = getNForSelect(socketN, 3);
    char message[size];
    memset(message, 0, size);
    struct timeval tv = {1, 0};

    while (portableCheckError(sproxySocket) == 0 && portableCheckError(telnetSocket) == 0)
    {
        reset(&readfds, telnetSocket->socket, sproxySocket->socket);
        struct timeval tv2 = {1, 0};
        selectVal = select(n, &readfds, NULL, NULL, &tv);
        if (selectVal == 0)
        {
            sendHeartbeat(sproxySocket);
            tv = tv2;
        }
        if (FD_ISSET(telnetSocket->socket, &readfds))
        {
            int result = forward(telnetSocket, sproxySocket, message, "telnet");
            if (result <= 0)
                break;
        }
        if (FD_ISSET(sproxySocket->socket, &readfds))
        {
            int result = recvMessage(sproxySocket, telnetSocket);
            if (result <= 0)
                break;
        }
        if (FD_ISSET(telnetAcceptorSocket->socket, &readfds))
        {
            printf("Detected new telnet session");
            portableClose(telnetSocket);
            telnetSocket = getTelnet(telnetAcceptorSocket);
            struct message reconnectStruct;
            char empty[0];
            empty[0] = '\0';
            createMessage(&reconnectStruct, NEW_CONNECTION, 0, empty);
            sendMessageStruct(&reconnectStruct, sproxySocket);
            int socketN[] = {sproxySocket->socket, telnetSocket->socket, telnetAcceptorSocket->socket};
            n = getNForSelect(socketN, 3);
        }
        if (lostHeartbeats > 3)
        {
            portableClose(sproxySocket);
            sproxySocket = getSproxy();
            struct message reconnectStruct;
            char empty[0];
            empty[0] = '\0';
            createMessage(&reconnectStruct, RECONNECT, 0, empty);
            sendMessageStruct(&reconnectStruct, sproxySocket);
            lostHeartbeats = 0;
        }
    }

    portableClose(telnetAcceptorSocket);
    portableClose(telnetSocket);
    portableClose(sproxySocket);
    portableCloseNetwork();
    return 0;
}
