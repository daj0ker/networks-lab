//Demonstrates unicasting, multicasting and broadcasting.
#include <stdio.h> 
#include <sys/socket.h> //required for socket, bind, listen, accept, socklen_t */
#include <arpa/inet.h> //required for sockaddr_in, inet_ntop 
#include <string.h> //for strlen
#include <stdlib.h> //required for atoi
#include <fcntl.h> //required for open, O_RDONLY 
#include <unistd.h> //required for close, read 
#include <signal.h> //signal handler. see the man page.
#include <sys/wait.h> //required for waitpid
#define SRV_PORT 9999 //default port
#define LISTEN_ENQ 5 //for listen backlog
#define MAX_RECV_BUF 256
#define MAX_SEND_BUF 256
#define SIZE 100

void get_file_name(int, char*);
int send_file(int , char*);
// bool unicast();
// bool multicast(const int max_num_clients, const int[] client_ids);
// bool broadcast(const int max_num_clients);

int main(int argc, char* argv[]) {
  int listen_fd, conn_fd; //listener and connect file descriptors.
  int num_clients = 0;
  struct sockaddr_in srv_addr, cli_addr;
  socklen_t cli_len;
  char file_name [MAX_RECV_BUF]; // file name
  char print_addr [INET_ADDRSTRLEN]; // readable IP address
  pid_t child_pid;
  int client[SIZE];
  
  //zero fill srv_addr and cli_addr
  memset(&srv_addr, 0, sizeof(srv_addr));
  memset(&cli_addr, 0, sizeof(cli_addr)); 
  srv_addr.sin_family = AF_INET; //use ipv4 protocol
  srv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //converts into big endian format aka network byte order
  // Use user-specified port number if available  
  srv_addr.sin_port = (argc > 1) ? htons(atoi(argv[1])) : htons(SRV_PORT);
  
  // Menu
  int choice;
  printf("Enter choice. \n1. Unicasting \n2. Multicasting \n3. Broadcasting \n");
  scanf("%d", &choice);
  
  if(choice != 1 && choice != 2 && choice != 3) {
    printf("Invalid option. Exiting.\n");
    return 0;
  }
  
  //Now that we've crossed the selection stage, let's setup the socket
  //-----------------SETUP---------------------------------
  //create socket
  if ( (listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
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
  //-----------------END-SETUP---------------------------------
  
  //--------------------UNICASTING----------------------------
  if(choice == 1) {
    
    //Wait for a client to connect; on successful connection, 
    //send the file, close the connection and stop listening
    printf("Entered unicast mode. \n");
    for( ; ; ) {
      cli_len = sizeof(cli_addr);
      printf("Waiting for a client to connect");
      if ( (conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len)) < 0 ) {
       perror("accept error");
       break; /* exit from the for loop */
      }

      inet_ntop(AF_INET, &(cli_addr.sin_addr), print_addr, INET_ADDRSTRLEN);
      printf("Client connected from %s:%d\n",
      print_addr, ntohs(cli_addr.sin_port) );
      
      get_file_name(conn_fd, file_name);
      int success = send_file(conn_fd, file_name);
      printf("Closing connection\n");
      close(conn_fd); //close socket after file transfer
      if(success != -1)
        break;
    }
    printf("Unicasting attempt complete. Program terminated.\n");
    close(listen_fd); 
    return 0;
  }
  //--------------------END-UNICASTING----------------------------
  
  //--------------------BROADCASTING-------------------------------
  else if (choice == 3) {
    int num_clients, curr_num_clients = 0;
    printf("Entered broadcast mode.\nEnter the max. number of clients: ");
    scanf("%d", &num_clients);
    int i = 0;
    for( ; ; ) {
       cli_len = sizeof(cli_addr);
       printf ("Waiting for a client to connect...\n\n");
       // wait for some client to connect 
       if ( (conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len)) < 0 ) {
         perror("accept error");
         break; 
       }
      //We've successfully connected to a client now.
      client[i++] = conn_fd;

      // convert numeric IP to readable format for displaying 
       inet_ntop(AF_INET, &(cli_addr.sin_addr), print_addr, INET_ADDRSTRLEN);
       printf("Client connected from %s:%d\n",
       print_addr, ntohs(cli_addr.sin_port) );
       curr_num_clients++;
       printf("Client Count: %d, %d clients haven't connected yet.\n", curr_num_clients,
        num_clients - curr_num_clients);
      
      if(curr_num_clients == num_clients) { //num_clients have connected.
        printf("All clients connected. Attempting broadcast\n");
        for(int i = 0; i < num_clients; i++) {
          printf("Attempting to send a file to client #%d\n", i + 1);
          get_file_name(client[i], file_name);
          send_file(client[i], file_name);
          printf("Closed connection from client #%d\n", i + 1);
          close(client[i]);
        }
        break;
      } 
    } 
    printf("Broadcasting attempt complete. Program terminated.\n");
    close(listen_fd); 
    return 0;
  }
  //--------------------END-BROADCASTING-------------------------------
  //--------------------MULTICASTING-----------------------------------
  else if (choice == 2) {
    int num_clients, curr_num_clients = 0;
    int should_send_to_client[SIZE];
    printf("Entered multicasting mode.\nEnter the max. number of clients: ");
    scanf("%d", &num_clients);
    printf("Enter a binary string corresponding to the list of clients\n");
    for(int i = 0; i < num_clients; i++) {
      scanf("%d", &should_send_to_client[i]);
    }

    int i = 0;
    for( ; ; ) {
       cli_len = sizeof(cli_addr);
       printf ("Waiting for a client to connect...\n\n");
       // wait for some client to connect  
       if ( (conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len)) < 0 ) {
         perror("accept error");
         break; 
       }
      //We've successfully connected to a client now.
      client[i++] = conn_fd;

      // convert numeric IP to readable format for displaying 
       inet_ntop(AF_INET, &(cli_addr.sin_addr), print_addr, INET_ADDRSTRLEN);
       printf("Client connected from %s:%d\n",
       print_addr, ntohs(cli_addr.sin_port) );
       curr_num_clients++;
       printf("Client Count: %d, %d clients haven't connected yet.\n", curr_num_clients,
        num_clients - curr_num_clients);
      
      if(curr_num_clients == num_clients) { //num_clients have connected.
        printf("All clients connected. Attempting multicasting\n");
        for(int i = 0; i < num_clients; i++) {
          if(should_send_to_client[i]) {
            printf("Attempting to send a file to client #%d\n", i + 1);
            get_file_name(client[i], file_name);
            send_file(client[i], file_name);
            printf("Closed connection from client #%d\n", i + 1);
            close(client[i]);
          }
          else {
            printf("Closed connection from client #%d\n", i + 1);
            close(client[i]);
          }
        }
        break;
      } 
    } 
    printf("Multicasting attempt complete. Program terminated.\n");
    close(listen_fd); 
    return 0;
  }

  /*
  for( ; ; ) {
     cli_len = sizeof(cli_addr);
     printf ("Waiting for a client to connect...\n\n");
     // wait for some client to connect  
     if ( (conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len)) < 0 ) {
       perror("accept error");
       break; 
     }
   
    // convert numeric IP to readable format for displaying 
     inet_ntop(AF_INET, &(cli_addr.sin_addr), print_addr, INET_ADDRSTRLEN);
     printf("Client connected from %s:%d\n",
     print_addr, ntohs(cli_addr.sin_port) );
     num_clients++;
     printf("Client Count: %d\n", num_clients);
     
     if ( (child_pid = fork()) == 0 ) { // fork returns 0 for child 
         close (listen_fd); // close child's copy of listen_fd 
         // do your work
         get_file_name(conn_fd, file_name);
         send_file(conn_fd, file_name);
         printf("Closing connection\n");
         // done 
         close(conn_fd); // close connected socket
         exit(0); // exit child process 
     }
    
    // get_file_name(conn_fd, file_name);
    // send_file(conn_fd, file_name);
    // printf("Closing connection\n");
    // close(conn_fd); //close socket after file transfer
  } 
 
  close(listen_fd); // close listening socket
  return 0;
  */
}
/**
 *function get_file_name
 * @param sock, the connection fd
 * @param file_name, the file name
 */
void get_file_name(int sock, char* file_name) {
  char recv_str[MAX_RECV_BUF]; 
  ssize_t rcvd_bytes; //number of bytes received
  
  //read file name from socket
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
  sent_bytes, // bytes sent to connected socket 
  sent_file_size; //sent file size
  char send_buf[MAX_SEND_BUF]; // max buffer size
  const char* errmsg_notfound = "File not found\n";
  int file_open_fd; // The corresponding file descriptor
  sent_count = 0;
  sent_file_size = 0;
  //attempt to open file
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

void sig_chld(int signo) { //for debugging
  pid_t pid;
  int stat;
  while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
  printf("child %d terminated\n", pid);
  return ;
}