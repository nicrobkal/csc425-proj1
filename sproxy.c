#include <stdio.h> 
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

/*
 * Sends all data packets in stream
 */
int sendAll(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 


int main(int argc, char *argv[]) 
{ 
    int cproxySocket = 0, daemonSocket = 0, cAccept = 0;
    int maxLen = 1025;
    struct sockaddr_in cproxyAddr = {0};
    int telnetAddrLen = sizeof(cproxyAddr);
    struct sockaddr_in daemonAddr = {0};
    fd_set readfds;
    char cproxyBuff[1025];
    char daemonBuff[1025];
    int opt = 1;

    //Check if arguments are valid
    if(argc != 2)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    while(1)
    {   
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
                close(cAccept);
                close(daemonSocket);
                return 1;
            }
            else if(rv == 0)
            {
                printf("Timeout occurred! No data after 10.5 seconds.\n");
                close(cAccept);
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
                        getpeername(cAccept, (struct sockaddr*)&cproxyAddr , (socklen_t*)&telnetAddrLen); 
                        printf("Host disconnected , ip %s , port %d \n" ,  
                            inet_ntoa(cproxyAddr.sin_addr) , ntohs(cproxyAddr.sin_port));
                        close(cAccept);
                        close(daemonSocket); 
                        break; 
                    }

                    if(strcmp(cproxyBuff, "exit") == 0 || strcmp(cproxyBuff, "logout") == 0){
                        close(cAccept);
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
                        int serverAddrLen = sizeof(daemonAddr);
                        getpeername(daemonSocket, (struct sockaddr*)&daemonAddr , (socklen_t*)&serverAddrLen); 
                        printf("Host disconnected , ip %s , port %d \n" ,  
                            inet_ntoa(daemonAddr.sin_addr) , ntohs(daemonAddr.sin_port));
                            close(daemonSocket);
                            close(cAccept);
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
