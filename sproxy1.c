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
    int serverSock = 0, newSocket = 0, serverFd = 0;
    int maxLen = 1025;
    struct sockaddr_in telnetAddr;
    int telnetAddrLen = sizeof(telnetAddr);
    struct sockaddr_in serverAddr;
    int addrLen = sizeof(serverAddr);
    fd_set readfds;
    int opt = 1;
    char telnetBuff[1025] = {0};
    char serverBuff[1025] = {0};

    //Check if arguments are valid
    if(argc != 2)
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
    serverAddr.sin_port = htons(23); 

    //Bind IP to socket
    if(inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <=0 )  
    { 
        printf("\nInvalid address. Terminating.\n"); 
        return 1; 
    } 
   
    //Connect to server
    if (connect(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return 1; 
    }

     //Create socket file descriptor
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        fprintf(stderr, "Socket failed to connect. Terminating.\n");
        return 1;
    } 
       
    //Attach socket to port
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        fprintf(stderr, "Failed setting sock options. Terminating.\n");
    return 1;
    }

    serverAddr.sin_family = AF_INET; 
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(atoi(argv[1])); 
       
    //Bind ip to socket
    if(bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        fprintf(stderr, "Binding failed. Terminating.\n");
        return 1;
    }
    
    //Enable listening on given socket
    if (listen(serverFd, 3) < 0) 
    { 
        fprintf(stderr, "Listening failed. Terminating.\n"); 
        exit(EXIT_FAILURE); 
    }

    //Accept the client
    if ((newSocket = accept(serverFd, (struct sockaddr *)&serverAddr, (socklen_t*)&addrLen))<0) 
    { 
        fprintf(stderr, "Accept failed. Terminating.\n");
        return 1;
    }

    //While user is still inputting data
    while(1) {
        //Clear the set
        int n = 0;
        FD_ZERO(&readfds);

        //Add descriptors
        FD_SET(newSocket, &readfds);
        FD_SET(serverSock, &readfds);

        //Find larger file descriptor
        if (newSocket > serverSock) {
            n = newSocket + 1;
        } else {
            n = serverSock + 1;
        }

        //Select returns one of the sockets or timeout
        int rv = select(n, &readfds, NULL, NULL, NULL);

        if (rv == -1) {
            perror("select");
            close(newSocket);
            close(serverSock);
            return 1;
        } else if (rv == 0) {
            printf("Timeout occurred! No data after 10.5 seconds.");
            close(newSocket);
            close(serverSock);
            return 1;
        } else {
            //One or both descrptors have data
            if (FD_ISSET(newSocket, &readfds)) {
                int valRead = recv(newSocket, telnetBuff, maxLen, 0);
                if (valRead == 0) {
                    getpeername(newSocket, (struct sockaddr *) &telnetAddr, (socklen_t * ) & telnetAddrLen);
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(telnetAddr.sin_addr), ntohs(telnetAddr.sin_port));
                    close(newSocket);
                }
                telnetBuff[valRead] = '\0';
                int telnetBuffLen = strlen(telnetBuff);
                send(serverSock, telnetBuff, telnetBuffLen, 0);
                printf("Telnet: %s", telnetBuff);
            }
            if (FD_ISSET(serverSock, &readfds)) {
                int valRead = recv(serverSock, serverBuff, maxLen, 0);
                if (valRead == 0) {
                    int serverAddrLen = sizeof(serverAddr);
                    getpeername(serverSock, (struct sockaddr *) &serverAddr, (socklen_t * ) & serverAddrLen);
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
                    close(serverSock);
                }
                serverBuff[valRead] = '\0';
                int serverBuffLen = strlen(serverBuff);
                send(newSocket, serverBuff, serverBuffLen, 0);
                printf("Server: %s", serverBuff);
                /*struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&serverAddr;
                struct in_addr ipAddr = pV4Addr->sin_addr;
                char str[INET_ADDRSTRLEN];
		        printf("Server: %s, %s", serverBuff, inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN));
                */
            }
        }

        //Sanitize buffers
        int i;
        for (i = 0; i < 1025; i++) {
            telnetBuff[i] = '\0';
            serverBuff[i] = '\0';
        }
    }
} 
