CS342 Network Lab Assignment- 3

#Compiling:

gcc server.c -o server

gcc client.c -o client (Run on multiple terminals to act as multiple clients)

#Running:

To start the server, open terminal and go to appropriate directory and write as follows
./server <port_number>
For example:  ./server 4000

To start the client, open terminal and go to appropriate directory and write as follows
./client <server ip address> <server port number>
Since running on local machine, add the server ip address as "127.0.0.1"
For example: ./client 127.0.0.1 4000 
	
#Properties

* TCP Sockets are used by both server and client to connect.
* Run the server before client/clients because only then will it be able to connect the server.
* In case of concurrent server, it uses (fork()) system calls to create child threads, to handle multiple clients concurrently.

#How it works

At first, clients try to connect to the running server using TCP connection. After successful connection, the client accepts text input from user and encodes the input using Base64 encoding system. After the encoding is done,client sends(Type 1 message) to the server via TCP port. After receiving message by the server, it decodes the message, and send an ACK(Type 2) message to the client. 
The client and server remains in loop to communicate any number of messages. When the user client inputs "exit", it means closing the connection with the server. Both server and client then closes the connection on their respective sides. 

#For client:
* To send message, enter in the following format <Message_type><space><Message>,    eg: "1 Hi"
* You will receive acknowledgement message from server in the format <2><space><ACK>,    eg: "2 ACK"
* To close connection, enter as follows, <3><space><exit>,    eg: "3 exit"

#For server:
* You will receive message from client in the following format <Message_type><space><Message>
* You need not input any message. Acknowledgements are automatically handled by server.

#Assumptions
* The length of the message to be sent by client will have length less than 2000 characters.
