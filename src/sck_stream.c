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

#define MAXHOSTNAME 1023
#define PORTNUM 0

void fireman(void);

// args: if binder calls this func => binderCaller = 1 else 0
int establish(unsigned short portnum, int binder_caller) {
  char myname[MAXHOSTNAME + 1];
  myname[MAXHOSTNAME] = '\0';
  int s;
  struct sockaddr_in sa;
  struct hostent *hp;

  memset(&sa, 0, sizeof(struct sockaddr_in)); // clear our addr
  gethostname(myname, MAXHOSTNAME); // who are we?
  
  hp = gethostbyname(myname); 


  //debug
  // printf("The host name is %s \n", myname);
  // printf("The h_name is %s \n", hp->h_name);
  // char hostname[1024];
  // strncpy(hostname, hp->h_name, 1024);
  // struct hostent *lh;
  // lh= gethostbyname(hostname);
  // printf("The h_name of this guys is %s\n", lh->h_name);
  //end-debug


  if (hp == NULL) {
    return (-1);
  }
  sa.sin_family = AF_INET; // our host address

  sa.sin_port = htons(portnum); // our port number

  //printf("The addr type is %d\n", hp->h_addrtype);


  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // create socket
    return (-1);
  }

  if (bind(s, &sa, sizeof(struct sockaddr_in)) < 0) { //bind address to socket
    close(s);
    return (-1);
  }

  listen(s, 5); //max # of queued connections

  // If the binder made the call, then print the address and port info
  if(binder_caller == 1) {
    socklen_t len = sizeof(sa);
    if (getsockname(s, &sa, &len) < 0) { 
      printf("getsockname error\n" );
    } else {
      printf("BINDER_ADDRESS %s\n", hp->h_name);
      printf("BINDER_PORT    %hu\n", sa.sin_port);
    }
  }
  return s;
}

int get_connection(int s) {
  int t;
  struct sockaddr_in cli_addr;
  socklen_t clilen;

  printf("Trying to connect to something... \n");

  clilen = sizeof(cli_addr);

  if ((t = accept(s, (struct sockaddr *) &cli_addr, &clilen)) < 0) { //accept connection if there is one
    printf("Not accepting bro.\n");
    return (-1);
  }

  printf("Accepted connection bro : %d\n", t);
  return t;
}

// client
int call_socket(char *hostname, unsigned short portnum)
{ struct sockaddr_in serv_addr;
  struct hostent     *server;
  int s;

  if ((server= gethostbyname(hostname)) == NULL) { /* do we know the host's */
    errno= ECONNREFUSED;                       /* address? */
    printf("Couldn't connect - hostname not found\n");
    return(-1);                                /* no */
  }

  memset(&serv_addr,0,sizeof(struct sockaddr_in));
  memcpy((char *)&serv_addr.sin_addr,server->h_addr,server->h_length); /* set address */

  serv_addr.sin_family= AF_INET;
  serv_addr.sin_port= htons((u_short)portnum);

  if ((s= socket(AF_INET,SOCK_STREAM,0)) < 0) {  /* get socket */
    printf("Couldn't connect - socket not created\n");
    return(-1);
  }

  printf("The client is using the socket %d\n", s);
  printf("The addr is %s\n", server->h_name);

  socklen_t len = sizeof(struct sockaddr_in);
  if (connect(s,&serv_addr,&len) < 0) {                  /* connect */
    printf("Couldn't connect - connection failed\n");
    close(s);
    return(-1);
  }
  return(s);
}

int read_data(int s,     /* connected socket */
              char *buf, /* pointer to the buffer */
              int n      /* number of characters (bytes) we want */
             )
{ int bcount; /* counts bytes read */
  int br;     /* bytes read this pass */

  bcount= 0;
  br= 0;
  while (bcount < n) {             /* loop until full buffer */
    if ((br= read(s,buf,n-bcount)) > 0) {
      bcount += br;                /* increment byte counter */
      buf += br;                   /* move buffer ptr for next read */
    }
    else if (br < 0)               /* signal an error to the caller */
      return(-1);
  }
  return(bcount);
}

int write_data(int s, char *buf, int n) {
  int bcount = 0;
  int br = 0;
  while (bcount < n) {
    if ((br = write(s, buf, n - bcount)) > 0) {
      bcount += br;
      buf += br;
    } else if (br < 0) {
      return -1;
    }
  }
  return(bcount);
}

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
void fireman(void)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}
