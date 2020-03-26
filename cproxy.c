#include <stdio.h> 
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

struct message
{
    char * payload;
    int type;
};

int NEW_CONN_TYPE;
int MESSAGE_TYPE;
int HEARTBEAT_TYPE;
int RECONNECT_TYPE;
int lostHeartbeats;

void sendStruct(int sock, struct message *msg)
{
    int msgSize = 10;
    char head[10] = {0};
    sprintf(head, "%d %d", msg->type, (int)strlen(msg->payload));
    
    char *buff = head;
    int length = msgSize;
    int i = 0;

    while (msgSize > 0)
    {
        if (msgSize <= 0)
        {
            break;
        }

        i = send(sock, buff, length, 0);

        if (i != 0)
        {
            break;
        }

        buff += i;
        length -= i;
    }

    buff = msg->payload;
    msgSize = strlen(msg->payload);
    length = msgSize;
    i = 0;

    while (msgSize > 0)
    {
        if (msgSize <= 0)
        {
            break;
        }

        i = send(sock, buff, length, 0);

        if (i != 0)
        {
            break;
        }

        buff += i;
        length -= i;
    }
}

int recvStruct(struct message *msg, int sender)
{
    char header[10];
    memset(header, 0, 10);
    if(recv(sender, header, 10, 0) < 0)
    {
        perror("recv");
        return -1;
    }

    int type;
    int len;
    sscanf(header, "%d %d", &type, &len);

    printf("Type: %d, Len: %d\n", type, len);

    if (len > 0)
    {
        if(recv(sender, msg->payload, strlen(msg->payload), 0) < 0)
        {
            perror("recv");
            return -1;
        }
    }
    
    msg->type = type;

    return strlen(msg->payload);
}

int main(int argc, char *argv[]) 
{ 
    int serverSock = 0, telnetAccept = 0, telnetSock = 0;
    int maxLen = 1025;
    struct sockaddr_in telnetAddr;
    int telnetAddrLen = sizeof(telnetAddr);
    struct sockaddr_in serverAddr;
    fd_set readfds;
    NEW_CONN_TYPE = 0;
    MESSAGE_TYPE = 1;
    HEARTBEAT_TYPE = 2;
    RECONNECT_TYPE = 3;
    lostHeartbeats = 0;

    //Check if arguments are valid
    if(argc != 4)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    //Create initial socket
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[2]);
    serverAddr.sin_port = htons(atoi(argv[3]));

    //Bind IP to socket
    if(inet_pton(AF_INET, argv[2], &serverAddr.sin_addr) < 0)  
    { 
        perror("inet_pton"); 
        return 1;
    }

    //Connect to server
    if (connect(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    { 
        perror("connect");
        return 1; 
    }

    //Create socket file descriptor
    if ((telnetAccept = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket");
        return 1;
    } 

    telnetAddr.sin_family = AF_INET; 
    telnetAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    telnetAddr.sin_port = htons(atoi(argv[1]));
    
    //Bind ip to socket
    if(bind(telnetAccept, (struct sockaddr *)&telnetAddr, sizeof(telnetAddr)) < 0) 
    {
        perror("bind");
        return 1;
    }

    //Enable listening on given socket
    if (listen(telnetAccept, 1) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }

    //Accept the client
    if ((telnetSock = accept(telnetAccept, (struct sockaddr *)&telnetAddr, (socklen_t*)&telnetAddrLen))<0) 
    { 
        perror("accept");
        return 1;
    }

    struct message newMessage;
    char buff[0];
    buff[0] = '\0';

    //Create initial handshake message
    newMessage.payload = buff;
    newMessage.type = NEW_CONN_TYPE;

    sendStruct(serverSock, &newMessage);

    int socketList[] = {serverSock, telnetAccept, telnetSock};

    int max = -1;
    int i;
    for (i = 0; i < 3; i++)
    {
        if (socketList[i] > max)
        {
            max = socketList[i];
        }
    }
    int n = max + 1;

    printf("Chum: %d, %d, %d, %d\n", serverSock, telnetAccept, telnetSock, n);

    char msg[1024] = {0};
    struct timeval tv = {1, 0};

    //While user is still inputting data
    while(1)
    {
        //Reset the socket descriptors
        FD_CLR(telnetAccept, &readfds);
        FD_CLR(serverSock, &readfds);
        FD_CLR(telnetSock, &readfds);
        FD_ZERO(&readfds);
        FD_SET(telnetAccept, &readfds);
        FD_SET(serverSock, &readfds);
        FD_SET(telnetSock, &readfds);

        if(FD_ISSET(telnetSock, &readfds))
        {
            printf("Shiz, n = %d\n", n);
        }

        //Set new timeval
        struct timeval tv1 = {1, 0};

        //Select returns one of the sockets or timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if (rv == 0) 
        {
            struct message newMsg;
            char buff[0];
            buff[0] = '\0';
            newMsg.payload = buff;
            newMsg.type = HEARTBEAT_TYPE;
            sendStruct(serverSock, &newMsg);
            tv = tv1;
            lostHeartbeats++;
        }
        if (FD_ISSET(telnetSock, &readfds))
        {
            int valRead = recv(telnetSock, msg, maxLen, 0);
            struct message newMsg;
            char buff[0];
            buff[0] = '\0';
            newMsg.payload = buff;
            newMsg.type = MESSAGE_TYPE;
            sendStruct(serverSock, &newMsg);
            if (valRead <= 0)
            {
                printf("Mehh");
                break;
            }
        }
        //3
        if (FD_ISSET(serverSock, &readfds)) 
        {
            struct message messStruct;
            char newMsg[1024];
            messStruct.payload = newMsg;
            int retVal = recvStruct(&messStruct, serverSock);
            if(messStruct.type == MESSAGE_TYPE)
            {
                char *buff = newMsg;
                int msgSize = strlen(newMsg);
                int length = msgSize;
                int i = 0;
                while (msgSize > 0)
                {
                    if (msgSize <= 0)
                    {
                        printf("Mehh1");
                        break;
                    }

                    i = send(telnetSock, buff, length, 0);

                    if (i != 0)
                    {
                        printf("Mehh2");
                        break;
                    }
                    buff += i;
                    length -= i;
                }
                memset(messStruct.payload, 0, strlen(messStruct.payload));
            }
            else if (messStruct.type == HEARTBEAT_TYPE)
            {
                lostHeartbeats = 0;
            }
            if(retVal <= 0)
            {
                printf("Mehh3: %d\n", retVal);
                break;
            }
        }
        if (FD_ISSET(telnetAccept, &readfds))
        {
            close(telnetSock);
            if ((telnetSock = accept(telnetAccept, (struct sockaddr *)&telnetAddr, (socklen_t*)&telnetAddrLen))<0) 
            {
                perror("accept");
                return 1;
            }
            struct message reconnectStruct;
            char buff[0];
            buff[0] = '\0';
            reconnectStruct.payload = buff;
            reconnectStruct.type = NEW_CONN_TYPE;
            printf("New Connection made.\n");
            sendStruct(serverSock, &reconnectStruct);
            int socketList[] = {serverSock, telnetSock, telnetAccept};
            int max = -1;
            int i;
            for (i = 0; i < 3; i++)
            {
                if (socketList[i] > max)
                {
                    max = socketList[i];
                }
            }
            n = max + 1;
        }
        if (lostHeartbeats > 3)
        {
            close(serverSock);
            
            //Recreate socket
            int serverSock = socket(PF_INET, SOCK_STREAM, 0);

            //Reconnect to server
            if (connect(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
            { 
                perror("connect");
                return 1; 
            }

            struct message reconnectStruct;
            char buff[0];
            buff[0] = '\0';
            reconnectStruct.payload = buff;
            reconnectStruct.type = RECONNECT_TYPE;
            sendStruct(serverSock, &reconnectStruct);
            lostHeartbeats = 0;
        }
    }

    close(serverSock);
    close(telnetAccept);
    close(telnetSock);

    return 0;
}
