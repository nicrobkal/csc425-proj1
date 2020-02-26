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
    int serverSock = 0, telnetSock = 0;
    int maxLen = 1024;
    int opt = 1; 
    struct sockaddr_in telnetAddr;
    int telnetAddrLen = sizeof(telnetAddr);
    struct sockaddr_in serverAddr;
    fd_set readfds;
    char telnetBuff[1025] = {0};
    char serverBuff[1025] = {0};

    //Check if arguments are valid
    if(argc != 4)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    //Create socket file descriptor
    if ((telnetSock = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket");
        return 1;
    } 
       
    //Attach socket to port
    if (setsockopt(telnetSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        perror("setsockopt");
    	return 1;
    }

    telnetAddr.sin_family = AF_INET; 
    telnetAddr.sin_addr.s_addr = INADDR_ANY; 
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
    if ((telnetSock = accept(telnetSock, (struct sockaddr *)&telnetAddr, (socklen_t*)&telnetAddrLen))<0) 
    { 
        perror("accept");
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

    /*
    //Bind IP to socket
    if(inet_pton(AF_INET, argv[2], &serverAddr.sin_addr) <=0 )  
    { 
        perror("inet_pton"); 
        return 1;
    } */
   
    //Connect to server
    if (connect(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    { 
        perror("connect");
        return 1; 
    }

    //While user is still inputting data
    while(1)
    {
        //Clear the set
        int n = 0;
        FD_ZERO(&readfds);

        //Add descriptors
        FD_SET(telnetSock, &readfds);
        FD_SET(serverSock, &readfds);

        //Find larger file descriptor
        if(telnetSock > serverSock)
        {
            n = telnetSock + 1;
        }
        else
        {
            n = serverSock + 1;
        }

        //Select returns one of the sockets or timeout
        int rv = select(n, &readfds, NULL, NULL, NULL);

        if (rv == -1)
        {
            fprintf(stderr, "Select() function failed.");
	        close(telnetSock);
	        close(serverSock);
	        return 1;
        }
        else if(rv == 0)
        {
            printf("Timeout occurred! No data after 10.5 seconds.");
            close(telnetSock);
	        close(serverSock);
	        return 1;
        }
        else
        {
            //One or both descrptors have data
            if(FD_ISSET(telnetSock, &readfds))
            {
                read(telnetSock, telnetBuff, maxLen);
                send(serverSock, telnetBuff, strlen(telnetBuff), 0);
		        printf("%s", telnetBuff);
            }
            if(FD_ISSET(serverSock, &readfds))
            {
                read(serverSock, serverBuff, maxLen);
                send(telnetSock, serverBuff, strlen(serverBuff), 0);
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
    }

    //Close the sockets
    close(telnetSock);
    close(serverSock);

    return 0; 
} 
