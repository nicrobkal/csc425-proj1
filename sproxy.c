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
int lostHeartbeats;

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

int recvStruct(struct message *msg, int sender)
{
    char header[16];
    memset(header, 0, 16);
    if(recv(sender, header, 16, 0) < 0)
    {
        perror("recv");
        return -1;
    }

    int type;
    int len;
    sscanf(header, "%d %d", &type, &len);

    if (len > 0)
    {
        if(recv(sender, msg->payload, strlen(msg->payload), 0) < 0)
        {
            perror("recv");
            return -1;
        }
    }
    
    msg->type = type;

    return len;
}

int main(int argc, char *argv[]) 
{ 
    int cproxySocket = 0, daemonSocket = 0, cAccept = 0;
    struct sockaddr_in cproxyAddr = {0};
    int telnetAddrLen = sizeof(cproxyAddr);
    struct sockaddr_in daemonAddr = {0};
    fd_set readfds;
    int opt = 1;
    int isClientConnected = 0;
    NEW_CONN_TYPE = 0;
    MESSAGE_TYPE = 1;
    HEARTBEAT_TYPE = 2;
    lostHeartbeats = 0;

    //Check if arguments are valid
    if(argc != 2)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    //Create socket file descriptor
    if ((cproxySocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket");
        return 1;
    } 

    //Attach socket to port
    if (setsockopt(cproxySocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        perror("setsockopt");
        return 1;
    }

    cproxyAddr.sin_family = AF_INET; 
    cproxyAddr.sin_addr.s_addr = INADDR_ANY;
    cproxyAddr.sin_port = htons(atoi(argv[1]));
    
    //Bind ip to socket
    if(bind(cproxySocket, (struct sockaddr *)&cproxyAddr, sizeof(cproxyAddr)) < 0)
    {
        perror("bind");
        return 1;
    }

    //Enable listening on given socket
    if (listen(cproxySocket, 1) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }

    //Accept the client
    if ((cAccept = accept(cproxySocket, (struct sockaddr *)&cproxyAddr, (socklen_t*)&telnetAddrLen))<0) 
    { 
        perror("accept");
        return 1;
    }

    isClientConnected = 1;

    struct message firstMessage;
    char buff[1024];
    buff[0] = '\0';

    //Create initial handshake message
    firstMessage.payload = buff;

    recvStruct(&firstMessage, cAccept);

    //Create initial socket
    if ((daemonSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket");
        return 1;
    }

    daemonAddr.sin_family = AF_INET;
    daemonAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    daemonAddr.sin_port = htons(23);

    //Bind IP to socket
    if(inet_pton(AF_INET, "127.0.0.1", &daemonAddr.sin_addr) <=0 )  
    { 
        perror("inet_pton"); 
        return 1;
    }

    //Connect to server
    if (connect(daemonSocket, (struct sockaddr *)&daemonAddr, sizeof(daemonAddr)) < 0) 
    { 
        perror("connect");
        return 1; 
    }

    int socketList[] = {daemonSocket, cproxySocket, cAccept};

    //Calculate n for select
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

    char msg[1024] = {0};
    struct timeval tv = {3, 0};

    printf("Before while");

    //While user is still inputting data
    while(1)
    {
        //Reset the sockets
        FD_CLR(daemonSocket, &readfds);
        FD_CLR(cproxySocket, &readfds);
        FD_CLR(cAccept, &readfds);
        FD_ZERO(&readfds);
        FD_SET(daemonSocket, &readfds);
        FD_SET(cproxySocket, &readfds);
        FD_SET(cAccept, &readfds);

        //Set new timevals
        struct timeval tv1 = {3, 0};
        struct timeval tv2 = {30, 0};

        //Select returns one of the sockets or timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if(FD_ISSET(daemonSocket, &readfds))
        {
            int valRead = recv(daemonSocket, msg, 1024, 0);
            if(isClientConnected == 1)
            {
                struct message newMsg;
                char buff[0];
                buff[0] = '\0';
                newMsg.payload = buff;
                newMsg.type = MESSAGE_TYPE;
                sendStruct(cAccept, &newMsg);
            }
            
            if (valRead <= 0) 
            {
                break;
            }
        }
        if(isClientConnected == 1 && FD_ISSET(cAccept, &readfds))
        {
            struct message messStruct;
            char newMsg[1024];
            messStruct.payload = newMsg;
            recvStruct(&messStruct, cAccept);

            if(messStruct.type == MESSAGE_TYPE)
            {
                char *buff = messStruct.payload;
                int msgSize = strlen(messStruct.payload);
                int length = msgSize;
                int i;

                while (msgSize > 0)
                {
                    if (msgSize <= 0)
                    {
                        break;
                    }

                    i = send(cAccept, buff, length, 0);

                    if (i != 0)
                    {
                        break;
                    }

                    buff += i;
                    length -= i;
                }
                memset(buff, 0, strlen(buff));
            }
            else if (messStruct.type == HEARTBEAT_TYPE)
            {
                struct message msgStruct;
                char buff[0];
                buff[0] = '\0';
                msgStruct.payload = buff;
                msgStruct.type = HEARTBEAT_TYPE;
                sendStruct(cAccept, &msgStruct);
            }
            else if (messStruct.type == NEW_CONN_TYPE)
            {
                close(cAccept);

                //Recreate socket
                int daemonSocket = socket(PF_INET, SOCK_STREAM, 0);

                int socketList[] = {daemonSocket, cAccept, cproxySocket};
                int max = -1;
                int i = 0;
                for (i = 0; i < 3; i++)
                {
                    if (socketList[i] > max)
                    {
                        max = socketList[i];
                    }
                }
                n = max + 1;
            }

            if(strlen(messStruct.payload) <= 0)
            {
                break;
            }

            tv = tv1;
        }
        if(FD_ISSET(cproxySocket, &readfds))
        {
            //Accept the client
            if ((cAccept = accept(cproxySocket, (struct sockaddr *)&cproxyAddr, (socklen_t*)&telnetAddrLen))<0) 
            { 
                perror("accept");
                return 1;
            }
            isClientConnected = 1;

            int socketList[] = {daemonSocket, cAccept, cproxySocket};
            int max = -1;
            int i = 0;
            for (i = 0; i < 3; i++)
            {
                if (socketList[i] > max)
                {
                    max = socketList[i];
                }
            }
            n = max + 1;

            struct message messStruct;
            char newMsg[1024];
            messStruct.payload = newMsg;
            recvStruct(&messStruct, cAccept);

            if(messStruct.type == NEW_CONN_TYPE)
            {
                close(cAccept);

                //Reconnect telnet
                daemonSocket = socket(PF_INET, SOCK_STREAM, 0);

                //Connect to server
                if (connect(daemonSocket, (struct sockaddr *)&daemonAddr, sizeof(daemonAddr)) < 0) 
                {
                    perror("connect");
                    return 1;
                }

                int newSocketList[] = {cAccept, cproxySocket};
                max = -1;
                for (i = 0; i < 2; i++)
                {
                    if (newSocketList[i] > max)
                    {
                        max = newSocketList[i];
                    }
                }
                n = max + 1;
            }
            
        }
        if(rv == 0 && isClientConnected == 0)
        {
            break;
        }
        if(rv == 0 && isClientConnected == 1)
        {
            close(cproxySocket);
            isClientConnected = 0;
            int socketList[] = {daemonSocket, cproxySocket, cAccept};
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

            tv = tv2;
        }
    }

    close(daemonSocket);
    close(cproxySocket);
    close(cAccept);

    return 0;
} 
