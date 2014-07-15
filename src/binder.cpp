/*
 * binder.cpp
 * 
 * This is the binder process
 */
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h" 

#define PORTNUM 0

#define PORT "0"
//void fireman(void);

using namespace std;

class BinderServer {
  private:
    int sockSelf; // socket that accepts connections from client and servers
    char* selfAddress;
    char* selfPort;

    static BinderServer* singleton;

  protected:
    BinderServer();

  public:
    static BinderServer* getInstance() {
      if (singleton == 0) {
        singleton = new BinderServer();
      }
      return singleton;
    }

    int startServer(); // start server and listen to server and client

    int terminate(){
      // terminate all the servers

      // terminate self - shut off the sockets and clean up memory used
      return 0;
    }


};


int main() {
  // int s, t;

  // if ((s= establish(PORTNUM, 1)) < 0) {  /* binder is calling to establish a listening socket */
  //   perror("establish");
  //   exit(1);
  // }

  // //printf("Binder started loopin'\n");
  // //signal(SIGCHLD, fireman);           /* this eliminates zombies */

  // for (;;) {                          /* loop for phone calls */
  //   if ((t= get_connection(s)) < 0) { /* get a connection */
  //     if (errno == EINTR)             /* EINTR might happen on accept(), */
  //       continue;                     /* try again */
  //     perror("accept");               /* bad */
  //     exit(1);
  //   }
  //   switch(fork()) {                  /* try to handle connection */
  //   case -1 :                         /* bad news.  scream and die */
  //     perror("fork");
  //     close(s);
  //     close(t);
  //     exit(1);
  //   case 0 :                          /* we're the child, do something */
  //     close(s);
  //     //work(t);
  //     exit(0);
  //   default :                         /* we're the parent so look for */
  //     close(t);                       /* another connection */
  //     continue;
  //   }
  // } 

  int sockfd, sockToClientfd;
  sockfd = setup_server("0", 1);
  sockToClientfd = wait_for_conn(sockfd);

  int a = 4;
  int b = 5;

  send(sockToClientfd, &a, sizeof(a), 0);
  send(sockToClientfd, &b, sizeof(b), 0);

  close(sockToClientfd);
  close(sockfd);

  return 0;

}

