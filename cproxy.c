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

int MESSAGE_TYPE;
int NEW_CONN_TYPE;
int HEARTBEAT_TYPE;

void sendStruct(int sock, struct message *msg)
{
    int msgSize = 16;
    char head[16] = {0};
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

int main(int argc, char *argv[]) 
{ 
    int serverSock = 0, telnetSock = 0, telnetAccept = 0;
    int maxLen = 1025;
    struct sockaddr_in telnetAddr;
    int telnetAddrLen = sizeof(telnetAddr);
    struct sockaddr_in serverAddr;
    fd_set readfds;
    char telnetBuff[1025] = {0};
    char serverBuff[1025] = {0};
    NEW_CONN_TYPE = 0;
    MESSAGE_TYPE = 1;
    HEARTBEAT_TYPE = 2;

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
    if(inet_pton(AF_INET, argv[2], &serverAddr.sin_addr) <=0 )  
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
    if ((telnetSock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket");
        return 1;
    } 

    telnetAddr.sin_family = AF_INET; 
    telnetAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    telnetAddr.sin_port = htons(atoi(argv[1]));
    
    //Bind ip to socket
    if(bind(telnetSock, (struct sockaddr *)&telnetAddr, sizeof(telnetAddr)) < 0) 
    {
        perror("bind");
        return 1;
    }

    //Enable listening on given socket
    if (listen(telnetSock, 1) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }

    //Accept the client
    if ((telnetAccept = accept(telnetSock, (struct sockaddr *)&telnetAddr, (socklen_t*)&telnetAddrLen))<0) 
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

    //While user is still inputting data
    while(1) {
        //Clear the set
        int n = 0;
        FD_ZERO(&readfds);

        //Add descriptors
        FD_SET(telnetAccept, &readfds);
        FD_SET(serverSock, &readfds);

        //Find larger file descriptor
        if (telnetAccept > serverSock) {
            n = telnetAccept + 1;
        } else {
            n = serverSock + 1;
        }

        //Select returns one of the sockets or timeout
        int rv = select(n, &readfds, NULL, NULL, NULL);

        if (rv == -1) {
            perror("select");
            close(telnetSock);
            close(serverSock);
            return 1;
        } else if (rv == 0) {
            printf("Timeout occurred! No data after 10.5 seconds.");
            close(telnetSock);
            close(serverSock);
            return 1;
        } else {
            //One or both descrptors have data
            if (FD_ISSET(telnetAccept, &readfds)) {
                int valRead = recv(telnetAccept, telnetBuff, maxLen, 0);
                if (valRead <= 0) {
                    getpeername(telnetAccept, (struct sockaddr *) &telnetAddr, (socklen_t * ) & telnetAddrLen);
                    printf("Host disconnected , ip %s , port %d \n",
                        inet_ntoa(telnetAddr.sin_addr), ntohs(telnetAddr.sin_port));
                    close(telnetSock);
                    close(serverSock);
                    break;
                }
                telnetBuff[valRead] = '\0';
                
                send(serverSock, telnetBuff, valRead, 0);

                if(strcmp(telnetBuff, "exit") == 0 || strcmp(telnetBuff, "logout") == 0){
                    close(telnetSock);
                    close(serverSock);
                    break;
                }
            }
            if (FD_ISSET(serverSock, &readfds)) {
                int valRead = recv(serverSock, serverBuff, maxLen, 0);
                if (valRead <= 0) {
                    int serverAddrLen = sizeof(serverAddr);
                    getpeername(serverSock, (struct sockaddr *) &serverAddr, (socklen_t * ) & serverAddrLen);
                    printf("Host disconnected , ip %s , port %d \n",
                        inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
                    close(telnetSock);
                    close(serverSock);
                    break;
                }
                serverBuff[valRead] = '\0';
                send(telnetAccept, serverBuff, valRead, 0);
            }
        }

        //Sanitize buffers
        int i;
        for (i = 0; i < 1025; i++) {
            telnetBuff[i] = '\0';
            serverBuff[i] = '\0';
        }
    }

    return 0;
}
