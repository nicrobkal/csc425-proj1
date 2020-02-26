#include <stdio.h> 
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

/*
 * Removes the newline character from a given string and returns a newly allocated string
 */
char* removeNewline(char *s)
{
    //Check if string is valid
    if(s[0] != '\0')
    {
        //Allocate space for new string
        char *n = malloc( strlen( s ? s : "\n" ) );

        //Copy string
        if(s)
        {
            strcpy( n, s );
        }

        if(n[strlen(n)-1] == '\n')
        {
            n[strlen(n)-1]='\0';
        }

        return n;
    }

    return s;
}

int main(int argc, char *argv[]) 
{ 
    int cproxySocket = 0, daemonSocket = 0;
    int maxLen = 256;
    int opt = 1; 
    struct sockaddr_in cproxyAddr = {0};
    int telnetAddrLen = sizeof(cproxyAddr);
    struct sockaddr_in daemonAddr = {0};
    fd_set readfds;
    char cproxyBuff[1025] = {0};
    char daemonBuff[1025] = {0};

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
    if ((cproxySocket = accept(cproxySocket, (struct sockaddr *)&cproxyAddr, (socklen_t*)&telnetAddrLen))<0) 
    { 
        perror("accept");
        return 1;
    }

    //Create initial socket
    if ((daemonSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        perror("socket"); 
        return 1;
    }
   
    daemonAddr.sin_family = AF_INET;
    daemonAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    daemonAddr.sin_port = htons(23); 

    /*//Bind IP to socket
    if(inet_pton(AF_INET, (struct sockaddr *)&serverAddr, &serverAddr.sin_addr) <=0 )  
    { 
        perror("inet_pton"); 
        return 1;
    }*/
   
    //Connect to server
    if (connect(daemonSocket, (struct sockaddr *)&daemonAddr, sizeof(daemonAddr)) < 0) 
    { 
        perror("connect");
        return 1; 
    }

    //fprintf(stderr, "Connected to telnet!\n");

    //While user is still inputting data
    while(1)
    {
        //Clear the set
        int n = 0;
        FD_ZERO(&readfds);

        //Add descriptors
        FD_SET(cproxySocket, &readfds);
        FD_SET(daemonSocket, &readfds);

        //fprintf(stderr, "cproxySocket: %d, daemonSocket: %d\n", cproxySocket, daemonSocket);

        //Find larger file descriptor
        if(cproxySocket > daemonSocket)
        {
            n = cproxySocket + 1;
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
            close(cproxySocket);
            close(daemonSocket);
            return 1;
        }
        else if(rv == 0)
        {
            printf("Timeout occurred! No data after 10.5 seconds.\n");
            close(cproxySocket);
            close(daemonSocket);
            return 1;
        }
        else
        {
            //One or both descrptors have data
            if(FD_ISSET(cproxySocket, &readfds))
            {
                int valRead = read(cproxySocket, cproxyBuff, maxLen);
                if(valRead == 0)
                {
                    getpeername(cproxySocket, (struct sockaddr*)&cproxyAddr , (socklen_t*)&telnetAddrLen); 
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(cproxyAddr.sin_addr) , ntohs(cproxyAddr.sin_port));
                    close(cproxySocket);    
                }
                cproxyBuff[valRead] = '\0';
                send(daemonSocket, cproxyBuff, strlen(cproxyBuff), 0);
                struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&daemonAddr;
                struct in_addr ipAddr = pV4Addr->sin_addr;
                char str[INET_ADDRSTRLEN];
		        //printf("Daemon from Cproxy: %s, %s\n", daemonBuff, inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN));
                //printf("Cproxy: %s", cproxyBuff);
            }
            if(FD_ISSET(daemonSocket, &readfds))
            {
                int valRead = read(daemonSocket, daemonBuff, maxLen);
                if(valRead == 0)
                {
                    int serverAddrLen = sizeof(daemonAddr);
                    getpeername(daemonSocket, (struct sockaddr*)&daemonAddr , (socklen_t*)&serverAddrLen); 
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(daemonAddr.sin_addr) , ntohs(daemonAddr.sin_port));
                        close(daemonSocket);
                }
                cproxyBuff[valRead] = '\0';
                send(cproxySocket, daemonBuff, strlen(daemonBuff), 0);
                struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&daemonAddr;
                struct in_addr ipAddr = pV4Addr->sin_addr;
                char str[INET_ADDRSTRLEN];
		        //printf("Daemon from daemon: %s, %s\n", daemonBuff, inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN));
                //printf("Daemon: %s", daemonBuff);
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

    /*
	//Convert input to network-readable language
    uint32_t temp = htonl(strlen(removeNewline(cproxyBuff)));
    
    //Checks if string is valid
    if(strlen(removeNewline(cproxyBuff)) > 0)
    {
        //Send the first packet holding the size of the coming message in bytes
        send(serverSock, &temp, 4, 0);

        //Sanitize input
        char* newBuff = removeNewline(cproxyBuff);

        //Send the actual message
        send(serverSock, newBuff, strlen(newBuff), 0); 

        //Check if packet was valid
        if(valRead < 0)
        {
            fprintf(stderr, "Failed to read from sock. Terminating.\n");
            return 1;
        }

        //Sanitizr cproxyBuff
        int i;
        for(i = 0; i < 1025; i++)
        {
            cproxyBuff[i] = '\0';
        }
    }*/

    //Close the sockets
    close(cproxySocket);
    close(daemonSocket);

    return 0; 
} 