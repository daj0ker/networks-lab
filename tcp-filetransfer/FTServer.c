// Single-threaded file transfer server
// TODO: Use fork() to serve multiple clients
//Reference: http://www.cse.iitk.ac.in/users/dheeraj/cs425/lec17.html
#include <stdio.h> 
#include <sys/socket.h> //required for socket, bind, listen, accept, socklen_t */
#include <arpa/inet.h> //required for sockaddr_in, inet_ntop 
#include <string.h> 
#include <stdlib.h> //required for atoi
#include <fcntl.h> //required for open, O_RDONLY 
#include <unistd.h> //required for close, read */
#define SRV_PORT 9999 // default port number */
#define LISTEN_ENQ 5 // for listen backlog
#define MAX_RECV_BUF 256
#define MAX_SEND_BUF 256

void get_file_name(int, char*);
int send_file(int , char*);

int main(int argc, char* argv[]) {
  int listen_fd, conn_fd; //listener and connect file descriptors
  struct sockaddr_in srv_addr, cli_addr;
  socklen_t cli_len;
  char file_name [MAX_RECV_BUF]; // file name
  char print_addr [INET_ADDRSTRLEN]; // readable IP address
  //zero fill srv_addr and cli_addr
  memset(&srv_addr, 0, sizeof(srv_addr));
  memset(&cli_addr, 0, sizeof(cli_addr)); 
  srv_addr.sin_family = AF_INET; //use ipv4 protocol
  srv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //converts into big endian format aka network byte order
  // Use user-specified port number if available  
  srv_addr.sin_port = (argc > 1) ? htons(atoi(argv[1])) : htons(SRV_PORT);
  
  //Create socket
  if ( (listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("Socket error");
    exit(EXIT_FAILURE);
  }
  
  //binding
  if( bind(listen_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0 ) {
    perror("bind error");
    exit(EXIT_FAILURE);
  }

  printf("Listening on port number %d ...\n", ntohs(srv_addr.sin_port));
  if( listen(listen_fd, LISTEN_ENQ) < 0 ) {
    perror("listen error");
    exit(EXIT_FAILURE);
  }

  for( ; ; ) {
     cli_len = sizeof(cli_addr);
     printf ("Waiting for a client to connect...\n\n");
     /* block until some client connects */
     if ( (conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len)) < 0 ) {
       perror("accept error");
       break; /* exit from the for loop */
     }
   
    /* convert numeric IP to readable format for displaying */
     inet_ntop(AF_INET, &(cli_addr.sin_addr), print_addr, INET_ADDRSTRLEN);
     printf("Client connected from %s:%d\n",
     print_addr, ntohs(cli_addr.sin_port) );
    
     get_file_name(conn_fd, file_name);
     send_file(conn_fd, file_name);
     printf("Closing connection\n");
     close(conn_fd); //close socket after file transfer
  } 
 
  close(listen_fd); /* close listening socket*/
  return 0;
}
/**
 *function get_file_name
 * @param sock, the connection fd
 * @param file_name, the file name
 */
void get_file_name(int sock, char* file_name) {
  char recv_str[MAX_RECV_BUF]; /* to store received string */
  ssize_t rcvd_bytes; /* bytes received from socket */
  
  /* read name of requested file from socket */
  if ( (rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0)) < 0) {
    perror("recv error");
    return;
  }
  
  sscanf (recv_str, "%s\n", file_name); 
}
/**
 *function send_file returns sent_count on success, -1 otherwise
 * @param sock, the connection fd
 * @param file_name, the file name
 */
int send_file(int sock, char *file_name) {
  
  int sent_count; //sent_count
  ssize_t read_bytes, //bytes read from file
  sent_bytes, // bytes sent to connected socket */
  sent_file_size; //sent file size
  char send_buf[MAX_SEND_BUF]; // max buffer size
  const char* errmsg_notfound = "File not found\n";
  int file_open_fd; // The corresponding file descriptor
  sent_count = 0;
  sent_file_size = 0;
  /* attempt to open requested file for reading */
  if( (file_open_fd = open(file_name, O_RDONLY)) < 0) { //Error in opening file
    perror(file_name);
    if( (sent_bytes = send(sock, errmsg_notfound ,
     strlen(errmsg_notfound), 0)) < 0 ) {
        perror("send error");
        return -1;
    }
  }

  else { //Successful open. 
    printf("Sending file: %s\n", file_name);
    while( (read_bytes = read(file_open_fd, send_buf, MAX_RECV_BUF)) > 0 ) {
      if( (sent_bytes = send(sock, send_buf, read_bytes, 0)) < read_bytes ) {
        perror("send error");
        return -1;
      }
      sent_count++;
      sent_file_size += sent_bytes;
    }
      close(file_open_fd);
  } 
  
  printf("Done with this client. Sent %d bytes in %d send(s)\n\n",
  sent_file_size, sent_count);
  return sent_count;
  
}