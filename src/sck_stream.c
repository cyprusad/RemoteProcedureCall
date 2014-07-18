#include <errno.h>       /* obligatory includes */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "message_protocol.h"
#include "response_codes.h"

#define MAXHOSTNAME 1023
#define PORTNUM 0
#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// // args: if binder calls this func => binderCaller = 1 else 0
// int establish(unsigned short portnum, int binder_caller) {
//   char myname[MAXHOSTNAME + 1];
//   myname[MAXHOSTNAME] = '\0';
//   int s;
//   struct sockaddr_in sa;
//   struct hostent *hp;

//   memset(&sa, 0, sizeof(struct sockaddr_in)); // clear our addr
//   gethostname(myname, MAXHOSTNAME); // who are we?
  
//   hp = gethostbyname(myname); 


//   //debug
//   // printf("The host name is %s \n", myname);
//   // printf("The h_name is %s \n", hp->h_name);
//   // char hostname[1024];
//   // strncpy(hostname, hp->h_name, 1024);
//   // struct hostent *lh;
//   // lh= gethostbyname(hostname);
//   // printf("The h_name of this guys is %s\n", lh->h_name);
//   //end-debug


//   if (hp == NULL) {
//     return (-1);
//   }
//   sa.sin_family = AF_INET; // our host address

//   sa.sin_port = htons(portnum); // our port number

//   //printf("The addr type is %d\n", hp->h_addrtype);


//   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // create socket
//     return (-1);
//   }

//   if (bind(s, &sa, sizeof(struct sockaddr_in)) < 0) { //bind address to socket
//     close(s);
//     return (-1);
//   }

//   listen(s, 5); //max # of queued connections

//   // If the binder made the call, then print the address and port info
//   if(binder_caller == 1) {
//     socklen_t len = sizeof(sa);
//     if (getsockname(s, &sa, &len) < 0) { 
//       printf("getsockname error\n" );
//     } else {
//       printf("BINDER_ADDRESS %s\n", hp->h_name);
//       printf("BINDER_PORT    %hu\n", sa.sin_port);
//     }
//   }
//   return s;
// }

// int get_connection(int s) {
//   int t;
//   struct sockaddr_in cli_addr;
//   socklen_t clilen;

//   printf("Trying to connect to something... \n");

//   clilen = sizeof(cli_addr);

//   if ((t = accept(s, (struct sockaddr *) &cli_addr, &clilen)) < 0) { //accept connection if there is one
//     printf("Not accepting bro.\n");
//     return (-1);
//   }

//   printf("Accepted connection bro : %d\n", t);
//   return t;
// }

int setup_server(char port[]) {
  int sockfd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  struct sigaction sa;
  int yes=1;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
          perror("server: socket");
          continue;
      }

      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
              sizeof(int)) == -1) {
          perror("setsockopt");
          exit(1);
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          perror("server: bind");
          continue;
      }

      break;
  }

  if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n");
      return 2;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }

  printf("server: waiting for connections...\n");

  return sockfd;

}

int addrAndPort(int sockfd, char hostname[], unsigned short* port) {
  struct sockaddr_in server;
  int addrlen = sizeof(server);
  if (getsockname(sockfd, (struct sockaddr *)&server,(socklen_t*) &addrlen) < 0) {
    printf("getsockname error\n");
  } else {
    if (gethostname(hostname, 128) == 0) {
      *port = htons(server.sin_port);
      return 0;
    }
  }
  return -1;
}


int wait_for_conn(int sockfd) {
    int sockToClientfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    sin_size = sizeof their_addr;
    sockToClientfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (sockToClientfd == -1) {
        perror("accept");
    }

    inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        s, sizeof s);
    printf("server: got connection from %s\n", s);

    return sockToClientfd;
}


int call_sock(char hostname[], char port[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            continue;
        }
        close(sockfd);

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}


// int read_data(int s,     /* connected socket */
//               char *buf, /* pointer to the buffer */
//               int n      /* number of characters (bytes) we want */
//              )
// { int bcount; /* counts bytes read */
//   int br;     /* bytes read this pass */

//   bcount= 0;
//   br= 0;
//   while (bcount < n) {             /* loop until full buffer */
//     if ((br= read(s,buf,n-bcount)) > 0) {
//       bcount += br;                 increment byte counter 
//       buf += br;                   /* move buffer ptr for next read */
//     }
//     else if (br < 0)               /* signal an error to the caller */
//       return(-1);
//   }
//   return(bcount);
// }


// blocking - read head
int* read_head(int sockfd) {
  int head[2];
  int nbytes;

  nbytes = recv(sockfd, head, sizeof(head), 0);
  // printf("read %d bytes\n", nbytes);
  // printf("read len: %d \nread type: %d \n", head[0], head[1]);

  return head;
}

//TODO consider writing a send all that sends everything we want to send (7.3 in Beej)

//blocking send calls (block till full message is sent)
int send_loc_request(int sockfd, char funcName[], int argTypes[], int sizeOfArgTypes) {
  printf("send_loc_request :: ");
  int len = (sizeof(char)*64) + sizeOfArgTypes*4; //funcName is is considered const size (max 64)
  printf("SEND:\nSending total of %d bytes\n", len);
  int head[2];
  head[0] = len;
  head[1] = RPC_LOC_REQUEST;

  printf("The funcname is: %s\nThe argType array len is: %d\n", funcName, sizeOfArgTypes);

  int bytesSent;

  bytesSent = send(sockfd, head, sizeof(head), 0);

  bytesSent = send(sockfd, funcName, (sizeof(char)*64), 0);

  bytesSent = send(sockfd, argTypes, sizeOfArgTypes*4, 0);

  return 0;
}

int send_terminate(int sockfd) {
  printf("send_terminate :: \n");
  int head[2];
  head[0] = 0;
  head[1] = RPC_TERMINATE;

  int bytesSent = send(sockfd, head, sizeof(head), 0);
  return 0; //error checking required
}

int send_execute_failure(int sockfd, int reasonCode) {
  printf("send_execute_failure :: \n");
  int exec_failure[3];
  exec_failure[0] = sizeof(int); //reasonCode is an int so 4 bytes
  exec_failure[1] = RPC_EXECUTE_FAILURE;
  exec_failure[2] = reasonCode;

  int bytesSent = send(sockfd, exec_failure, sizeof(exec_failure), 0);
  return 0;
}

int send_loc_success(int sockfd, char hostname[], unsigned short port) {
  printf("send_loc_success :: \n");
  int head[2];
  head[0] = (sizeof(char)*128) + sizeof(unsigned short);
  head[1] = RPC_LOC_SUCCESS;
 
  int bytesSent;

  bytesSent = send(sockfd, head, sizeof(head), 0);

  bytesSent = send(sockfd, hostname, (sizeof(char)*128), 0);

  bytesSent = send(sockfd, &port, sizeof(unsigned short), 0);

  return BS_SUCC_SEND_CLIENT;

}

int send_loc_failure(int sockfd, int reasonCode) {
  printf("send_loc_failure :: \n");
  int loc_failure[3];
  loc_failure[0] = sizeof(int); //reasonCode is an int so 4 bytes
  loc_failure[1] = RPC_LOC_FAILURE;
  loc_failure[2] = reasonCode;

  int bytesSent = send(sockfd, loc_failure, sizeof(loc_failure), 0);
  return BS_SUCC_SEND_CLIENT;
}

int send_register(int sockfd, char server_identifier[], unsigned short port, char funcName[], int argTypes[], int sizeOfArgTypes) {
  printf("send_register :: \n");
  int len = sizeof(char)*128 + sizeof(unsigned short)*2 + sizeof(char)*64 + sizeOfArgTypes*4;

  printf("SEND:\nSending total of %d bytes\n", len);
  int head[2];
  head[0] = len;
  head[1] = RPC_REGISTER;

  printf("The server registered is: %s\nThe port of server is: %u\nThe func name is: %s\nThe first elem of argTypes is: %d\n", server_identifier, port, funcName, argTypes[0]);

  int bytesSent;

  bytesSent = send(sockfd, head, sizeof(head), 0);

  bytesSent = send(sockfd, server_identifier, sizeof(char)*128, 0);

  bytesSent = send(sockfd, &port, sizeof(unsigned short), 0);

  bytesSent = send(sockfd, funcName, sizeof(char)*64, 0);

  bytesSent = send(sockfd, argTypes, sizeOfArgTypes*4, 0);

  return 0;

}

int send_register_success(int sockfd, int warningFlag) {
  printf("send_register_success :: \n");
  int reg_success[3];
  reg_success[0] = sizeof(int); //warningFlag is an int so 4 bytes
  reg_success[1] = RPC_REGISTER_SUCCESS;
  reg_success[2] = warningFlag;

  int bytesSent = send(sockfd, reg_success, sizeof(reg_success), 0);

  // if the send happened successfully then BS_SUCC_SEND_SERVER   
  return BS_SUCC_SEND_SERVER;
}

int send_register_failure(int sockfd, int reasonCode) {
  printf("send_register_failure :: \n");
  int reg_failure[3];
  reg_failure[0] = sizeof(int); //reasonCode is an int so 4 bytes
  reg_failure[1] = RPC_REGISTER_FAILURE;
  reg_failure[2] = reasonCode;

  int bytesSent = send(sockfd, reg_failure, sizeof(reg_failure), 0);

  return BS_SUCC_SEND_SERVER;
}
