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
    int sock = 0;
    int valRead = 0; 
    struct sockaddr_in servAddr; 
    char buffer[1025] = {0};
    char *tempBuff = malloc(1025);

    //Check if arguments are valid
    if(argc != 3)
    {
        fprintf(stderr, "Arguments invalid. Terminating.\n");
        return 1;
    }

    //Create initial socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("Failed to create socket. Terminating.\n"); 
        return 1; 
    } 
   
    servAddr.sin_family = AF_INET; 
    servAddr.sin_port = htons(atoi(argv[2])); 

    //Bind IP to socket
    if(inet_pton(AF_INET, argv[1], &servAddr.sin_addr) <=0 )  
    { 
        printf("\nInvalid address. Terminating.\n"); 
        return 1; 
    } 
   
    //Connect to server
    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return 1; 
    }

    //While user is still inputting data
    while((tempBuff = fgets(buffer, 1025, stdin)) > 0)
    {
        //Convert input to network-readable language
        uint32_t temp = htonl(strlen(removeNewline(buffer)));
        
        //Checks if string is valid
        if(strlen(removeNewline(buffer)) > 0)
        {
            //Send the first packet holding the size of the coming message in bytes
            send(sock, &temp, 4, 0);

            //Sanitize input
            char* newBuff = removeNewline(buffer);

            //Send the actual message
            send(sock, newBuff, strlen(newBuff), 0); 

            //Check if packet was valid
            if(valRead < 0)
            {
                fprintf(stderr, "Failed to read from socket. Terminating.\n");
                return 1;
            }

            //Sanitizr buffer
            int i;
            for(i = 0; i < 1025; i++)
            {
                buffer[i] = '\0';
            }
        }
    }

    //Close the socket
    close(sock);

    return 0; 
} 
