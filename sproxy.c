#include <stdio.h> 
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

int connectToDaemon()
{
    int daemonSocket = 0;
    struct sockaddr_in daemonAddr;

    //Create initial socket
    if ((daemonSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) 
    {
        perror("socket");
        return -1;
    }

    daemonAddr.sin_family = AF_INET;
    daemonAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    daemonAddr.sin_port = htons(23);

    //Bind IP to socket
    if(inet_pton(AF_INET, "127.0.0.1", &daemonAddr.sin_addr) <= 0)  
    {
        perror("inet_pton");
        return -1;
    }

    //Connect to server
    if (connect(daemonSocket, (struct sockaddr *)&daemonAddr, sizeof(daemonAddr)) < 0)
    { 
        perror("connect");
        return -1; 
    }

    return daemonSocket;
}

int acceptClientConnection(int* cproxySocket, struct sockaddr_in* cproxyAddr, char* targetPort)
{
    int cAccept = 0, opt = 1;

    //Create socket file descriptor
    if ((*cproxySocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) 
    { 
        perror("socket");
        return -1;
    } 

    //Attach socket to port
    if (setsockopt(*cproxySocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    { 
        perror("setsockopt");
        return -1;
    }

    cproxyAddr->sin_family = AF_INET; 
    cproxyAddr->sin_addr.s_addr = INADDR_ANY;
    cproxyAddr->sin_port = htons(atoi(targetPort));
    
    //Bind ip to socket
    if(bind(*cproxySocket, (struct sockaddr *)cproxyAddr, sizeof(*cproxyAddr)) < 0)
    {
        perror("bind");
        return -1;
    }

    //Enable listening on given socket
    if (listen(*cproxySocket, 1) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }

    //Accept the client
    if ((cAccept = accept(*cproxySocket, (struct sockaddr *)cproxyAddr, (socklen_t*)sizeof(cproxyAddr))) < 0) 
    { 
        perror("accept");
        return -1;
    }

    return cAccept;
}

int main(int argc, char *argv[]) 
{ 
    int* cproxySocket = malloc(sizeof(int));
    *cproxySocket = 0;
    int daemonSocket = 0, cAccept = 0;
    int maxLen = 1025;
    struct sockaddr_in cproxyAddr;
    fd_set readfds;
    char cproxyBuff[1025];
    char daemonBuff[1025];

    //Check if arguments are valid
    if(argc != 2)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    while(1)
    {
        daemonSocket = connectToDaemon();

        if(daemonSocket == -1)
        {
            return -1;
        }

        cAccept = acceptClientConnection(cproxySocket, &cproxyAddr, argv[1]);

        if(cAccept == -1)
        {
            return -1;
        }

        //While user is still inputting data
        while(1)
        {
            //Clear the set
            int n = 0;
            FD_ZERO(&readfds);

            //Add descriptors
            FD_SET(cAccept, &readfds);
            FD_SET(daemonSocket, &readfds);

            //Find larger file descriptor
            if(cAccept > daemonSocket)
            {
                n = cAccept + 1;
            }
            else
            {
                n = daemonSocket + 1;
            }

            //Select returns one of the sockets or timeout
            int rv = select(n, &readfds, NULL, NULL, NULL);


            if (rv == -1)
            {
                perror("select");
                close(*cproxySocket);
                close(daemonSocket);
                return 1;
            }
            else if(rv == 0)
            {
                printf("Timeout occurred! No data after 10.5 seconds.\n");
                close(*cproxySocket);
                close(daemonSocket);
                return 1;
            }
            else
            {
                //One or both descrptors have data
                if(FD_ISSET(cAccept, &readfds))
                {
                    int valRead = recv(cAccept, cproxyBuff, maxLen, 0);
                    if(valRead <= 0)
                    {
                        close(*cproxySocket);
                        close(daemonSocket); 
                        break; 
                    }

                    if(strcmp(cproxyBuff, "exit") == 0 || strcmp(cproxyBuff, "logout") == 0)
                    {
                        close(*cproxySocket);
                        close(daemonSocket);
                        break;
                    }
                    send(daemonSocket, cproxyBuff, valRead, 0);
                }
                if(FD_ISSET(daemonSocket, &readfds))
                {
                    int valRead = recv(daemonSocket, daemonBuff, maxLen, 0);
                    if(valRead <= 0)
                    {
                        //getpeername(daemonSocket, (struct sockaddr*)daemonAddr , (socklen_t*)sizeof(daemonAddr)); 
                        //printf("Host disconnected , ip %s , port %d \n" ,  
                            //inet_ntoa(daemonAddr->sin_addr) , ntohs(daemonAddr->sin_port));
                            close(daemonSocket);
                            close(*cproxySocket);
                            break;
                    }
                    send(cAccept, daemonBuff, valRead, 0);

                }
            }

            //Sanitize buffers
            int i;
            for(i = 0; i < 1025; i++)
            {
                cproxyBuff[i] = '\0';
                daemonBuff[i] = '\0';
            }
        }
    }   
} 
