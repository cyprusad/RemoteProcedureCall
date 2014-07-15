/*
 * binder.cpp
 * 
 * This is the binder process
 */

#include <errno.h>       /* obligatory includes */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

#include "rpc.h"
#include "socket_stream.h" 

#define PORTNUM 0
//void fireman(void);


int terminate(){
  // terminate all the servers

  // terminate self - shut off the sockets and clean up memory used
  return 0;
}



int main() {
  int s, t;

  if ((s= establish(PORTNUM, 1)) < 0) {  /* binder is calling to establish a listening socket */
    perror("establish");
    exit(1);
  }

  //printf("Binder started loopin'\n");
  //signal(SIGCHLD, fireman);           /* this eliminates zombies */

  for (;;) {                          /* loop for phone calls */
    if ((t= get_connection(s)) < 0) { /* get a connection */
      if (errno == EINTR)             /* EINTR might happen on accept(), */
        continue;                     /* try again */
      perror("accept");               /* bad */
      exit(1);
    }
    switch(fork()) {                  /* try to handle connection */
    case -1 :                         /* bad news.  scream and die */
      perror("fork");
      close(s);
      close(t);
      exit(1);
    case 0 :                          /* we're the child, do something */
      close(s);
      //work(t);
      exit(0);
    default :                         /* we're the parent so look for */
      close(t);                       /* another connection */
      continue;
    }
  } 

  return 0;

}

