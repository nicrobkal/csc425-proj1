#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

/*
 * Checks for a value read failure when recv'ing from socket.
 */
void valReadError(int valRead)
{
    if(valRead < 0)
    {
        fprintf(stderr, "Failed to read from socket. Terminating.\n");
        exit(1);
    }
}

/*
 * Resets the buffer for the incoming message
 */
void resetBuffer(char* buffer)
{
    int i;
    for(i = 0; i < 1025; i++)
    {
        buffer[i] = '\0';
    }
}

int main(int argc, char *argv[]) 
{ 
    int serverFd; 
    int newSocket; 
    int valRead; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrLen = sizeof(address); 
    char buffer[1025] = {0};
    uint32_t messageLen;
    int len; 
       
    //Check if arguments are valid
    if(argc != 2)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
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

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(atoi(argv[1])); 
       
    //Bind ip to socket
    if(bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0) 
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
    if ((newSocket = accept(serverFd, (struct sockaddr *)&address, (socklen_t*)&addrLen))<0) 
    { 
        fprintf(stderr, "Accept failed. Terminating.\n");
        return 1;
    }

    //While socket is open and sending data
    while((valRead = recv(newSocket, buffer, 1024, 0)) > 0)
    {
        //Convert to host-readable language
        valReadError(valRead);

	printf("%s", buffer);
        
	resetBuffer(buffer);
        //If length of message requires two recv's
        /*if(len > 536)
        {
            //Read first part and print
            valRead = recv(newSocket, buffer, 536, 0);
            valReadError(valRead);
            printf("%s", buffer); 
            resetBuffer(buffer);

            //Read second part and print
            valRead = recv(newSocket, buffer, len - 536 + 1, 0);
            valReadError(valRead);
            printf("%s", buffer);
            resetBuffer(buffer);
            
        }
        else if(len > 0)
        {
            //Read message and print
            valRead = recv(newSocket, buffer, len + 1, 0);
            valReadError(valRead);
            printf("%s", buffer); 
            resetBuffer(buffer);
        }*/
        //Reset message length
        messageLen = 0;
    }

    return 0; 
} 
