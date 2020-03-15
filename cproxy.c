/*
 * Client-side program to create seamless connection for a telnet session intending to move between WiFi networks
 */

#include "portablesocket.h"
#include <sys/select.h>
#include "message.h"

int selectValue;
int clientPort;
char *serverAddress;
int serverPort;
int heartbeatsSinceLastReply;
struct PortableSocket *telnetAcceptorSocket;
struct PortableSocket *telnetSocket;
struct PortableSocket *sproxySocket;
int n;

//gets the value of n for select
int getN(int socket[], int numberOfSockets)
{
    int max = -1;
    int i = 0;
    for (i = 0; i < numberOfSockets; i++)
    {
        if (socket[i] > max)
            max = socket[i];
    }
    return max + 1;
}

//parses the input
void parseInput(int argc, char *argv[])
{
    int current = 1;
    selectValue = 0;
    clientPort = atoi(argv[current++]);
    serverAddress = argv[current++];
    serverPort = atoi(argv[current++]);
}

//get the telnetAcceptorSocket
struct PortableSocket *getTelnetAcceptor()
{
    struct PortableSocket *telnetAcceptorSocket = cpSocket(100, "127.0.0.1", clientPort);
    if (cpCheckError(telnetAcceptorSocket) != 0)
    {
        fprintf(stderr, "Failed to create telnet acceptor socket \n");
        exit(1);
    }
    cpBind(telnetAcceptorSocket);
    cpListen(telnetAcceptorSocket, 5);
    return telnetAcceptorSocket;
}

//get the telnetSocket
struct PortableSocket *getTelnet(struct PortableSocket *telnetAcceptorSocket)
{
    struct PortableSocket *telnetSocket = cpAccept(telnetAcceptorSocket);
    if (cpCheckError(telnetSocket) != 0)
    {
        fprintf(stderr, "Failed to create telnet socket \n");
        exit(1);
    }
    return telnetSocket;
}

struct PortableSocket *getSproxy()
{
    struct PortableSocket *sproxySocket = cpSocket(100, serverAddress, serverPort);
    cpConnect(sproxySocket);
    if (cpCheckError(sproxySocket) != 0)
    {
        fprintf(stderr, "Failed to create sproxy socket\n");
        exit(1);
    }
    return sproxySocket;
}

// resets the select method, to be used again
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

//forwards a message from the sender socket to the reciever socket
int forward(struct PortableSocket *sender, struct PortableSocket *reciever, char *message, char *senderName)
{
    // print "recieved from telnet 'message' sending to sproxy"
    int messageSize = cpRecv(sender, message, 1024);
    if (cpCheckError(sender) != 0)
        return -1;
    struct message messageStruct;
    initMessageStruct(&messageStruct, MESSAGE, messageSize, message);
    sendMessageStruct(&messageStruct, reciever);
    return messageSize;
}

//forwards a message from the sender socket to the reciever socket
int sendMessage(struct PortableSocket *reciever, char *message, int messageSize)
{
    cpSend(reciever, message, messageSize);
    memset(message, 0, messageSize);
    return 0;
}

int recvMessage(struct PortableSocket *sender, struct PortableSocket *reciever)
{
    struct message messageStruct;
    char message[1024];
    messageStruct.payload = message;
    recvMessageStruct(&messageStruct, sender);
    if (messageStruct.type == MESSAGE)
    {
        sendMessage(reciever, messageStruct.payload, messageStruct.length);
    }
    else if (messageStruct.type == HEARTBEAT)
    {
        printf("recived heartbeat reply\n");
        heartbeatsSinceLastReply = 0;
        return 1;
    }
    return messageStruct.length;
}

void sendHeartbeat(struct PortableSocket *reciever)
{
    heartbeatsSinceLastReply++;
    struct message messageStruct;
    char empty[0];
    empty[0] = '\0';
    initMessageStruct(&messageStruct, HEARTBEAT, 0, empty);
    sendMessageStruct(&messageStruct, reciever);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        return 1;
    }

    /*
  * Parse the inputs
  */
    parseInput(argc, argv);

    /*
  * Connection to the local telnet
  */
    telnetAcceptorSocket = getTelnetAcceptor();
    telnetSocket = getTelnet(telnetAcceptorSocket);

    /*
  * Create connection to sproxy
  */
    sproxySocket = getSproxy();
    struct message newConnectStruct;
    char empty[0];
    empty[0] = '\0';
    initMessageStruct(&newConnectStruct, NEW_CONNECTION, 0, empty);
    sendMessageStruct(&newConnectStruct, sproxySocket);
    /*
  * set up data for the program
  */
    fd_set readfds;
    int socketN[] = {sproxySocket->socket, telnetSocket->socket, telnetAcceptorSocket->socket};
    n = getN(socketN, 3);
    char message[1024];
    memset(message, 0, 1024);
    struct timeval tv = {1, 0};
    /*
  * run the program
  */
    while (cpCheckError(sproxySocket) == 0 && cpCheckError(telnetSocket) == 0)
    {
        reset(&readfds, telnetSocket->socket, sproxySocket->socket);
        struct timeval tv2 = {1, 0};
        selectValue = select(n, &readfds, NULL, NULL, &tv);
        if (selectValue == 0)
        {
            sendHeartbeat(sproxySocket);
            tv = tv2;
        }
        // foward the message
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
            cpClose(telnetSocket);
            telnetSocket = getTelnet(telnetAcceptorSocket);
            struct message reconnectStruct;
            char empty[0];
            empty[0] = '\0';
            initMessageStruct(&reconnectStruct, NEW_CONNECTION, 0, empty);
            sendMessageStruct(&reconnectStruct, sproxySocket);
            int socketN[] = {sproxySocket->socket, telnetSocket->socket, telnetAcceptorSocket->socket};
            n = getN(socketN, 3);
        }
        if (heartbeatsSinceLastReply > 3)
        {
            cpClose(sproxySocket);
            sproxySocket = getSproxy();
            struct message reconnectStruct;
            char empty[0];
            empty[0] = '\0';
            initMessageStruct(&reconnectStruct, RECONNECT, 0, empty);
            sendMessageStruct(&reconnectStruct, sproxySocket);
            heartbeatsSinceLastReply = 0;
        }
    }

    /*
    * Close the connections
    */
    cpClose(telnetAcceptorSocket);
    cpClose(telnetSocket);
    cpClose(sproxySocket);
    cpCloseNetwork();
    return 0;
}
