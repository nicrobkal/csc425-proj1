#include <stdio.h> 
#include <sys/socket.h> 
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
    int masterSocket = 0, serverSock = 0;
    int maxLen = 256;
    int opt = 1; 
    struct sockaddr_in telnetAddr = {0};
    int telnetAddrLen = sizeof(telnetAddr);
    struct sockaddr_in serverAddr = {0};
    int serverAddrLen = sizeof(serverAddr);
    fd_set readfds;
    char telnetBuff[1025] = {0};
    char serverBuff[1025] = {0};

    //Check if arguments are valid
    if(argc != 2)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    //Create socket file descriptor
    if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        fprintf(stderr, "Socket failed to connect. Terminating.\n");
        return 1;
    } 
       
    //Attach socket to port
    if (setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        fprintf(stderr, "Failed setting sock options. Terminating.\n");
    	return 1;
    }

    telnetAddr.sin_family = AF_INET; 
    telnetAddr.sin_addr.s_addr = INADDR_ANY; 
    telnetAddr.sin_port = htons(atoi(argv[1])); 
       
    //Bind ip to socket
    if(bind(masterSocket, (struct sockaddr *)&telnetAddr, sizeof(telnetAddr)) < 0) 
    {
        fprintf(stderr, "Server binding failed. Terminating.\n");
        return 1;
    }
    
    //Enable listening on given socket
    if (listen(masterSocket, 3) < 0) 
    { 
        fprintf(stderr, "Listening failed. Terminating.\n"); 
        exit(EXIT_FAILURE); 
    }

    //Accept the client
    if ((masterSocket = accept(masterSocket, (struct sockaddr *)&telnetAddr, (socklen_t*)&telnetAddrLen))<0) 
    { 
        fprintf(stderr, "Server accept failed. Terminating.\n");
        return 1;
    }

    fprintf(stderr, "Connected to cproxy!");

    //Create socket file descriptor
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        fprintf(stderr, "Socket failed to connect. Terminating.\n");
        return 1;
    } 
       
    //Attach socket to port
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        fprintf(stderr, "Failed setting sock options. Terminating.\n");
    	return 1;
    }

    serverAddr.sin_family = AF_INET; 
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htonl(23);
  
    //Bind ip to socket
    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        perror("bind");
        return 1;
    }
    
    //Enable listening on given socket
    if (listen(serverSock, 3) < 0) 
    { 
        fprintf(stderr, "Listening failed. Terminating.\n"); 
        exit(EXIT_FAILURE); 
    }

    fprintf(stderr, "Waiting on something to accept...");

    //Accept the client
    if ((serverSock = accept(serverSock, (struct sockaddr *)&serverAddr, (socklen_t*)&serverAddrLen))<0) 
    { 
        fprintf(stderr, "Telnet accept failed. Terminating.\n");
        return 1;
    }

    fprintf(stderr, "Connected to something else!");

    //While user is still inputting data
    while(1)
    {
        //Clear the set
        int n = 0;
        FD_ZERO(&readfds);

        //Add descriptors
        FD_SET(masterSocket, &readfds);
        FD_SET(serverSock, &readfds);

        //Find larger file descriptor
        if(masterSocket > serverSock)
        {
            n = masterSocket + 1;
        }
        else
        {
            n = serverSock + 1;
        }

        //Select returns one of the sockets or timeout
        int rv = select(n, &readfds, NULL, NULL, NULL);

        if (rv == -1)
        {
            fprintf(stderr, "Select() function failed.\n");
            close(masterSocket);
            close(serverSock);
            return 1;
        }
        else if(rv == 0)
        {
            printf("Timeout occurred! No data after 10.5 seconds.\n");
            close(masterSocket);
            close(serverSock);
            return 1;
        }
        else
        {
            //One or both descrptors have data
            if(FD_ISSET(masterSocket, &readfds))
            {
                recv(masterSocket, telnetBuff, maxLen, 0);
                send(serverSock, telnetBuff, strlen(telnetBuff), 0);
                printf("%s", telnetBuff);
            }
            if(FD_ISSET(serverSock, &readfds))
            {
                recv(serverSock, serverBuff, maxLen, 0);
                send(masterSocket, serverBuff, strlen(serverBuff), 0);
                printf("%s", serverBuff);
            }
        }

            //Sanitize buffers
        int i;
        for(i = 0; i < 1025; i++)
        {
            telnetBuff[i] = '\0';
            serverBuff[i] = '\0';
        }
    }

    /*
	//Convert input to network-readable language
    uint32_t temp = htonl(strlen(removeNewline(telnetBuff)));
    
    //Checks if string is valid
    if(strlen(removeNewline(telnetBuff)) > 0)
    {
        //Send the first packet holding the size of the coming message in bytes
        send(serverSock, &temp, 4, 0);

        //Sanitize input
        char* newBuff = removeNewline(telnetBuff);

        //Send the actual message
        send(serverSock, newBuff, strlen(newBuff), 0); 

        //Check if packet was valid
        if(valRead < 0)
        {
            fprintf(stderr, "Failed to read from sock. Terminating.\n");
            return 1;
        }

        //Sanitizr telnetBuff
        int i;
        for(i = 0; i < 1025; i++)
        {
            telnetBuff[i] = '\0';
        }
    }*/

    //Close the sockets
    close(masterSocket);
    close(serverSock);

    return 0; 
} 
