#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include "base64_encoder.c"
#include "base64_decoder.c"
#define MSG_LEN 2000

//Create a Socket for client communication
short SocketCreate(void)
{
    short socketid;
    printf("Creating the socket..\n");
    socketid = socket(AF_INET, SOCK_STREAM, 0);
    return socketid;
}

//connect client socket to server socket
int SocketConnect(int socket, char *server_ip, int ServerPort)
{
    int iRetval= -1;
    struct sockaddr_in remote;
    bzero(&remote, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(server_ip); 
    remote.sin_port = htons(ServerPort);
    iRetval = connect(socket,(struct sockaddr *)&remote,sizeof(remote));
    return iRetval;
}

void chat(int socket, char *server_ip, int ServerPort) 
{ 
    char buff[MSG_LEN]; 
    for (;;) { 
        bzero(buff, sizeof(buff)); 
        printf("Enter your message: "); 
        scanf("%[^\n]%*c", buff);
        char *ans;
        ans = (char *)malloc(sizeof(char)*MSG_LEN);
        bzero(ans, sizeof(ans));
        ans = encode(buff+2, (unsigned int)(strlen(buff))-2);
        bzero(buff+2, (size_t)((unsigned int)sizeof(buff)-2));
        for(unsigned int i=0; i< (unsigned int)strlen(ans); i++){
            buff[i+2] = ans[i];
        }
        write(socket, buff, strlen(buff));

        if (buff[0] == '3') { 
            printf("EXITING CLIENT\n");
            return; 
        } 

        bzero(buff, sizeof(buff)); 
        read(socket, buff, sizeof(buff)); 
        ans = decode(buff+2, (int)strlen(buff)-2);
        // printf("%s\n%s\n", buff, ans);
        bzero(buff+2, (size_t)((int)sizeof(buff)-2));
        
        for(unsigned int i=0; i< (unsigned int)strlen(ans); i++){
            buff[i+2] = ans[i];
        }
        if(buff[0] != '2'){
            printf("Acknowledgement not received, Please resend your message\n");
            continue;
        } 
        else if(buff[0] == '2'){
            printf("Message received from server %s and port %d: ", server_ip, ServerPort);
            printf("%s\n", buff+2);
            continue;
        }
    } 
} 
  
int main(int argc, char *argv[]) 
{ 
    int socket; 
    char *server_ip = argv[1]; //server ip address, input from user
    int ServerPort = atoi(argv[2]); //server socket address, input from user

    // socket create and verification 
    socket = SocketCreate();
    if (socket == -1) { 
        printf("SOCKET CREATION FAILED\n"); 
        exit(0); 
    } 
    else{
        printf("SOCKET SUCCESFULLY CREATED\n");
    } 

    if(SocketConnect(socket, server_ip, ServerPort) != 0){
        printf("CONNECTION FAILED..\n");
        exit(0);
    }
    else{
        printf("CONNECTION ESTABLISHED..\n"); 
    }
    // function for chat 
    chat(socket, server_ip, ServerPort); 
  
    // close the socket 
    close(socket); 
} 