#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>
#include "base64_encoder.c"
#include "base64_decoder.c"
#define MSG_LEN 2000

//Create a Socket for server communication
short SocketCreate(void)
{
    short socketid;
    printf("Creating the socket..\n");
    socketid = socket(AF_INET, SOCK_STREAM, 0);
    return socketid;
}

// Function designed for chat between client and server. 
void chat(int clientsocket, struct sockaddr_in *client) 
{ 
    char buff[MSG_LEN];

    char *ip_address = inet_ntoa(client->sin_addr); //ip address of client
    int PORT = client->sin_port; //port of client
    printf("CONNECTION ESTABLISHED with %s and PORT %d\n", ip_address, PORT);

    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MSG_LEN); 
  
        
        read(clientsocket, buff, sizeof(buff)); // read the message from client and copy it in buffer 
         
        printf("Message Received from client %s with port %d \n", ip_address, PORT); // print buffer which contains the client contents
        printf("\tEncoded message: %s\n", buff);
        char *temp;
        temp = decode(buff+2, (unsigned int)strlen(buff)-2);
        bzero(buff+2, sizeof(buff)-2);
        for(unsigned int i=0; i< (unsigned int)strlen(temp); i++){
            buff[i+2] = temp[i];
        }
        printf("\tDecoded message: %s\n", buff); 
        if (buff[0] == '3') { 
            break; 
        } 
        else if(buff[0] = '1'){
            bzero(buff, sizeof(buff));
            buff[0] = '2';
            buff[1] = ' ';
            temp = encode("ACK", 3);
            for(unsigned int i=0; i<(unsigned int)strlen(temp); i++){
                buff[i+2] = temp[i];
            }
            write(clientsocket, buff, strlen(buff));
        }
        else{
           bzero(buff, sizeof(buff));
            buff[0] = '2';
            buff[1] = ' ';
            temp = encode("UNKNOWN MESSAGE TYPE", 20);
            for(unsigned int i=0; i<(unsigned int)strlen(temp); i++){
                buff[i+2] = temp[i];
            }
            write(clientsocket, buff, strlen(buff)); 
        }
    }
    free(decoding_table);
    
    close(clientsocket); // After chatting close the socket 
    printf("CONNECTION CLOSED with %s and PORT %d\n", ip_address, PORT); 
    exit(0); 
} 
  
// Driver function 
int main(int argc, char *argv[]) 
{ 
    int PORT = atoi(argv[1]);
    int socket, clientsocket, len; 
    struct sockaddr_in server, client; 

    
    socket = SocketCreate(); // socket create and verification 
    if (socket == -1) { 
        printf("SOCKET CREATION FAILED\n"); 
        exit(0); 
    } 
    else{
        printf("SOCKET SUCCESFULLY CREATED\n");
    } 
     bzero(&server, sizeof(server)); 
     
    // assign IP, PORT 
    server.sin_family = AF_INET; 
    server.sin_addr.s_addr = htonl(INADDR_ANY); 
    server.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(socket, (struct sockaddr*)&server, sizeof(server))) != 0) { 
        printf("socket binding failed\n"); 
        close(socket);
        exit(0); 
    } 
    else{
        printf("Socket binded successful\n"); 
    }

    // Now server is ready to listen and verification 
    if ((listen(socket, 5)) != 0) { 
        printf("LISTEN FAILED\n"); 
        close(socket);
        exit(0); 
    } 
    else{ 
        printf("SERVER LISTENING\n");
    } 

    len = sizeof(client); 
   
    for(;;){
        
        clientsocket = accept(socket, (struct sockaddr*)&client, &len); // Accept the data packet from client and verification 

        //connection failed
        if (clientsocket < 0) { 
            printf("CONNECTION COULD NOT BE ESTABLISHED WITH SERVER\n"); 
            exit(0); 
        } 

        int ret = fork();
        if(ret < 0){
            printf("CONNECTION FAILED\n");
        }
        else if(ret == 0){
            close(socket); // since serversocket was handled by parent process

            chat(clientsocket, &client); // Function for chatting between client and server 
        }
        else{
            close(clientsocket); //since client socket to be handled by child process
        }
    } 
} 