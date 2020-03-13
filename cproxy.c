#include <stdio.h> 
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

/*
 * Function to connect to the server
 *  Returns the socket value
 */
int connectToServer(char* targetIP, char* targetPort)
{
    int sproxySocket = 0;
    struct sockaddr_in sproxyAddr;

    //Create initial socket
    if ((sproxySocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) 
    {
        perror("socket");
        return -1;
    }

    sproxyAddr.sin_family = AF_INET;
    sproxyAddr.sin_addr.s_addr = inet_addr(targetIP);
    sproxyAddr.sin_port = htons(atoi(targetPort));

    //Bind IP to socket
    if(inet_pton(AF_INET, targetIP, &sproxyAddr.sin_addr) < 0)
    {
        perror("inet_pton");
        return -1;
    }

    //Connect to server
    if (connect(sproxySocket, (struct sockaddr*)&sproxyAddr, sizeof(sproxyAddr)) < 0) 
    {
        perror("connect");
        return -1;
    }

    return sproxySocket;
}

/*
 * Function to create a new socket for telnet and accept an incoming connection
 *  Returns the socket value
 */
int acceptTelnetConnection(int* telnetSocket, struct sockaddr_in* telnetAddr, char* targetPort)
{
    int telnetAccept = 0;

    //Create socket file descriptor
    if ((*telnetSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
        perror("socket");
        return -1;
    }

    telnetAddr->sin_family = AF_INET; 
    telnetAddr->sin_addr.s_addr = inet_addr("127.0.0.1");
    telnetAddr->sin_port = htons(atoi(targetPort));
    
    //Bind ip to socket
    if(bind(*telnetSocket, (struct sockaddr *)telnetAddr, sizeof(*telnetAddr)) < 0) 
    {
        perror("bind");
        return -1;
    }

    //Enable listening on given socket
    if (listen(*telnetSocket, 1) < 0) 
    { 
        perror("listen");
        exit(EXIT_FAILURE);
    }

    socklen_t telnetAddrLen = sizeof(telnetAddr);

    //Accept the client
    if ((telnetAccept = accept(*telnetSocket, (struct sockaddr *)telnetAddr, &telnetAddrLen)) < 0) 
    {
        perror("accept");
        return -1;
    }

    return telnetAccept;
}

unsigned char * serializeInt(unsigned char *buffer, int value)
{
    buffer[0] = value >> 24;
    buffer[1] = value >> 16;
    buffer[2] = value >> 8;
    buffer[3] = value;
    return buffer + 4;
}

unsigned char * serializeChar(unsigned char *buffer, char value)
{
    buffer[0] = value;
    return buffer + 1;
}

unsigned char * serializeString(unsigned char *buffer, char *value)
{
    int i;
    for(i = 0; i < strlen(value); i++)
    {
        serializeChar(buffer, value[i]);
    }

    return buffer;
}

unsigned char * serializeMessage(unsigned char *buffer, int header, char * payload)
{
    buffer = serializeInt(buffer, header);
    buffer = serializeString(buffer, payload);
    return buffer;
}

unsigned char * deserializeMessage(unsigned char *buffer, int *header)
{
    header[0] += buffer[0] << 24;
    header += buffer[1] << 16;
    header += buffer[2] << 8;
    header += buffer[3];
    return buffer + 4;
}

int main(int argc, char *argv[]) 
{ 
    int sproxySocket = 0, telnetAccept = 0;
    int* telnetSocket = malloc(sizeof(int));
    *telnetSocket = 0;
    int maxLen = 1025;
    struct sockaddr_in telnetAddr;
    fd_set datareadfds, heartbeatreadfds;
    char telnetBuff[1025] = {0};
    char sproxyBuff[1025] = {0};
    int telnetConnected = 1;
    struct timeval tv;

    //Check if arguments are valid
    if(argc != 4)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    while(1)
    {
        FD_ZERO(&heartbeatreadfds);
        
        //Add descriptors
        FD_SET(telnetAccept, &heartbeatreadfds);
        FD_SET(sproxySocket, &heartbeatreadfds);

        sproxySocket = connectToServer(argv[2], argv[3]);

        if(sproxySocket == -1)
        {
            return 1;
        }

        if(telnetConnected != 0)
        {
            telnetAccept = acceptTelnetConnection(telnetSocket, &telnetAddr, argv[1]);

            if(telnetAccept == -1)
            {
                return 1;
            }

            telnetConnected = 1;
        }

        //While user is still inputting data
        while(1)
        {
            //Clear the set
            int n = 0;
            FD_ZERO(&datareadfds);

            //Add descriptors
            FD_SET(telnetAccept, &datareadfds);
            FD_SET(sproxySocket, &datareadfds);

            //Find larger file descriptor
            if (telnetAccept > sproxySocket)
            {
                n = telnetAccept + 1;
            }
            else
            {
                n = sproxySocket + 1;
            }

            tv.tv_sec = 1;

            //Select returns one of the sockets or timeout
            int rv = select(n, &datareadfds, NULL, NULL, &tv);

            if (rv == -1) 
            {
                perror("select");
                close(*telnetSocket);
                close(sproxySocket);
                return 1;
            } 
            else if (rv == 0) 
            {
                printf("Timeout occurred! No data after 10.5 seconds.");
                close(*telnetSocket);
                close(sproxySocket);
                return 1;
            } 
            else 
            {
                //One or both descrptors have data
                if (FD_ISSET(telnetAccept, &datareadfds)) 
                {
                    int valRead = recv(telnetAccept, telnetBuff, maxLen, 0);
                    if (valRead <= 0) 
                    {
                        close(*telnetSocket);
                        close(sproxySocket);
                        break;
                    }
                    telnetBuff[valRead] = '\0';
                    
                    send(sproxySocket, telnetBuff, valRead, 0);

                    if(strcmp(telnetBuff, "exit") == 0 || strcmp(telnetBuff, "logout") == 0)
                    {
                        close(*telnetSocket);
                        close(sproxySocket);
                        break;
                    }
                }
                if (FD_ISSET(sproxySocket, &datareadfds))
                {
                    int valRead = recv(sproxySocket, sproxyBuff, maxLen, 0);
                    if (valRead <= 0)
                    {
                        close(*telnetSocket);
                        close(sproxySocket);
                        break;
                    }
                    sproxyBuff[valRead] = '\0';
                    send(telnetAccept, sproxyBuff, valRead, 0);
                }
            }

            //Sanitize buffers
            int i;
            for (i = 0; i < 1025; i++)
            {
                telnetBuff[i] = '\0';
                sproxyBuff[i] = '\0';
            }
        }
    }
} 
