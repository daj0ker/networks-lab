//Reference: http://www.cse.iitk.ac.in/users/dheeraj/cs425/lec17.html
#include <stdio.h> 
#include <sys/socket.h> // socket, connect, socklen_t 
#include <arpa/inet.h> // sockaddr_in, inet_pton 
#include <string.h>  
#include <stdlib.h> // atoi
#include <fcntl.h> // O_WRONLY, O_CREAT 
#include <unistd.h> // close, write, read 
#define SRV_PORT 9999
#define MAX_RECV_BUF 256
#define MAX_SEND_BUF 256
int recv_file(int ,char*);

int main(int argc, char* argv[]) {
  
   int sock_fd;
   struct sockaddr_in srv_addr;

   if (argc < 3) {
     printf("usage: %s <filename> <IP address> [port number]\n", argv[0]);
     exit(EXIT_FAILURE);
   }
   
   memset(&srv_addr, 0, sizeof(srv_addr)); // zero-fill srv_addr structure. This isn't required though.

   // Create the client socket.
   sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   srv_addr.sin_family = AF_INET; // Use ipv4

   //parse the command line arg IP adddr.
   if ( inet_pton(AF_INET, argv[2], &(srv_addr.sin_addr)) < 1 ) {
      printf("Invalid IP address\n");
      exit(EXIT_FAILURE);
   }

   //Check if the user has specified the portno. If not, use the default port
   srv_addr.sin_port = (argc > 3) ? htons(atoi(argv[3])) : htons(SRV_PORT);

   //establish connection
   if( connect(sock_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0 ) {
     perror("connect error");
     exit(EXIT_FAILURE);
   }
   
   printf("connected to:%s:%d ..\n", argv[2], SRV_PORT);

   recv_file(sock_fd, argv[1]); /* argv[1] = file name */

   //close socket
   if(close(sock_fd) < 0) {
     perror("socket close error");
     exit(EXIT_FAILURE);
   }
   
   return 0;
}
/**
 * function recv_file
 * @param sock, the connection fd
 * @param file_name, the file name
 * @return the received file size
 */
int recv_file(int sock, char* file_name) {
 
    char send_str [MAX_SEND_BUF]; //the message 
    int file_open_fd; /* file handle for receiving file*/
    ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
    int recv_count; /* count of recv() calls*/
    char recv_str[MAX_RECV_BUF]; /* buffer to hold received data */
    size_t send_strlen; /* length of transmitted string */

    sprintf(send_str, "%s\n", file_name); /* add CR/LF (new line) */
    send_strlen = strlen(send_str); /* length of message to be transmitted */
    if( (sent_bytes = send(sock, file_name, send_strlen, 0)) < 0 ) {
        perror("send error");
        return -1;
    }
    
    // attempt to create file to save received data. 0644 = rw-r--r-- 
    // refer to https://www.linux.com/learn/understanding-linux-file-permissions
    if ( (file_open_fd = open(file_name, O_WRONLY|O_CREAT, 0644)) < 0 ) {
        perror("error creating file");
        return -1;
    }
   
    recv_count = 0; // number of recv() calls required to receive the file 
    rcvd_file_size = 0; // size of received file 

    //decide when to terminate 
    while ( (rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0)) > 0 ) {
        recv_count++;
        rcvd_file_size += rcvd_bytes;

        if (write(file_open_fd, recv_str, rcvd_bytes) < 0 ) {
            perror("error writing to file");
            return -1;
        }
    }
    
    close(file_open_fd); 
    printf("Client Received: %d bytes in %d recv(s)\n", rcvd_file_size,
    recv_count);
    return rcvd_file_size;
}