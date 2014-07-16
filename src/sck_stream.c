#include <errno.h>       /* obligatory includes */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "message_protocol.h"

#define MAXHOSTNAME 1023
#define PORTNUM 0
#define BACKLOG 10     // how many pending connections queue will hold

void fireman(void);

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

  // // If the binder made the call, then print the address and port info
  // char addr_str[INET6_ADDRSTRLEN];
  // struct sockaddr_in srv;
  // int addrlen = sizeof(srv);
  // if(binder_caller == 1) {    
  //   if (getsockname(sockfd, (struct sockaddr *)&srv, &addrlen) < 0) { 
  //     printf("getsockname error\n" );
  //   } else {
  //     char alternate[1024];
  //     inet_ntop(srv.sin_family, get_in_addr((struct sockaddr *) &srv.sin_addr), addr_str, sizeof(addr_str));
  //     if (gethostname(alternate, 1024) == 0) {
  //       printf("BINDER_ADDRESS %s\n", alternate);
  //     }
      
  //     printf("BINDER_PORT    %hu\n", htons(srv.sin_port));
  //   }
  // }

  return sockfd;

}

int addrAndPort(int sockfd, char hostname[], unsigned short* port) {
  struct sockaddr_in server;
  int addrlen = sizeof(server);
  if (getsockname(sockfd, (struct sockaddr *)&server, &addrlen) < 0) {
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

// // client
// int call_socket(char *hostname, unsigned short portnum)
// { struct sockaddr_in serv_addr;
//   struct hostent     *server;
//   int s;

//   if ((server= gethostbyname(hostname)) == NULL) { /* do we know the host's */
//     errno= ECONNREFUSED;                       /* address? */
//     printf("Couldn't connect - hostname not found\n");
//     return(-1);                                /* no */
//   }

//   memset(&serv_addr,0,sizeof(struct sockaddr_in));
//   memcpy((char *)&serv_addr.sin_addr,server->h_addr,server->h_length); /* set address */

//   serv_addr.sin_family= AF_INET;
//   serv_addr.sin_port= htons((u_short)portnum);

//   if ((s= socket(AF_INET,SOCK_STREAM,0)) < 0) {  /* get socket */
//     printf("Couldn't connect - socket not created\n");
//     return(-1);
//   }

//   printf("The client is using the socket %d\n", s);
//   printf("The addr is %s\n", server->h_name);

//   socklen_t len = sizeof(struct sockaddr_in);
//   if (connect(s,&serv_addr,&len) < 0) {                  /* connect */
//     printf("Couldn't connect - connection failed\n");
//     close(s);
//     return(-1);
//   }
//   return(s);
// }

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
            close(sockfd);
            perror("client: connect");
            continue;
        }

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


// major work to be done here
int read_message(int sockfd) {
  int size_head = 8;
  int head[2];
  int len, type;
  int nbytes;

  nbytes = recv(sockfd, head, sizeof(head), 0);
  printf("read %d bytes\n", nbytes);
  printf("read len: %d \nread type: %d \n", head[0], head[1]);

  return 0;
}

int send_terminate(int sockfd) {
  int head[2];
  head[0] = 0;
  head[1] = RPC_TERMINATE;

  int bytesSent = send(sockfd, head, sizeof(head), 0);
  return 0; //error checking required
}

int send_execute_failure(int sockfd, int reasonCode) {
  int exec_failure[3];
  exec_failure[0] = 4; //reasonCode is an int so 4 bytes
  exec_failure[1] = RPC_EXECUTE_FAILURE;
  exec_failure[2] = reasonCode;

  int bytesSent = send(sockfd, exec_failure, &exec_failure, 0);
  return 0;
}

int send_loc_success(int sockfd, char hostname[], int port) {
  // have a fixed size for hostname say something like 128 bytes (chars) and 4 bytes for port

}

int send_loc_failure(int sockfd, int reasonCode) {
  int loc_failure[3];
  loc_failure[0] = 4; //reasonCode is an int so 4 bytes
  loc_failure[1] = RPC_LOC_FAILURE;
  loc_failure[2] = reasonCode;

  int bytesSent = send(sockfd, loc_failure, &loc_failure, 0);
  return 0;
}

// int write_data(int s, char *buf, int n) {
//   int bcount = 0;
//   int br = 0;
//   while (bcount < n) {
//     if ((br = write(s, buf, n - bcount)) > 0) {
//       bcount += br;
//       buf += br;
//     } else if (br < 0) {
//       return -1;
//     }
//   }
//   return(bcount);
// }

// int main() {
//   int s, t;

//   if ((s= establish(PORTNUM, 0)) < 0) {  /* plug in the phone */
//     perror("establish");
//     exit(1);
//   }

//   signal(SIGCHLD, fireman);           /* this eliminates zombies */

//   for (;;) {                          /* loop for phone calls */
//     if ((t= get_connection(s)) < 0) { /* get a connection */
//       if (errno == EINTR)             /* EINTR might happen on accept(), */
//         continue;                     /* try again */
//       perror("accept");               /* bad */
//       exit(1);
//     }
//     switch(fork()) {                  /* try to handle connection */
//     case -1 :                         /* bad news.  scream and die */
//       perror("fork");
//       close(s);
//       close(t);
//       exit(1);
//     case 0 :                          /* we're the child, do something */
//       close(s);
//       //work(t);
//       exit(0);
//     default :                         /* we're the parent so look for */
//       close(t);                       /* another connection */
//       continue;
//     }
//   } 

//   return 0;
// }

/* as children die we should get catch their returns or else we get
 * zombies, A Bad Thing.  fireman() catches falling children.
 */
// void fireman(void)
// {
//   while (waitpid(-1, NULL, WNOHANG) > 0)
//     ;
// }
