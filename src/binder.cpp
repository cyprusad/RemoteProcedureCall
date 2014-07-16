/*
 * binder.cpp
 * 
 * This is the binder process
 */
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h" 
//#include "binder.h"

#define PORTNUM 0

#define PORT "0"
//void fireman(void);

using namespace std;

class BinderServer {
  private:
    int sockSelfFd; // socket that accepts connections from client and servers
    
    // who am i
    char selfAddress[128];
    unsigned short selfPort;

    static BinderServer* singleton;

  protected:
    //BinderServer();

  public:
    static BinderServer* getInstance() {
      if (singleton == 0) {
        singleton = new BinderServer;
      }
      return singleton;
    }

    int startServer() {
      sockSelfFd = setup_server("0");

      unsigned short* portPtr = &selfPort;
      if (addrAndPort(sockSelfFd, selfAddress, portPtr) == 0) {
        printf("BINDER_ADDRESS %s\n", selfAddress);
        printf("BINDER_PORT    %hu\n", selfPort);
      } else {
        printf("warning[0]: binder doesn't know its hostname/port; possible that binder didn't start properly");
      }

      return sockSelfFd; //TODO error checking here
    }

    int terminate(){
      // terminate all the servers

      // terminate self - shut off the sockets and clean up memory used
      return 0;
    }


};


BinderServer* BinderServer::singleton = NULL;

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

  // int argTypes[3];
  // argTypes[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16); 
  // argTypes[1] = (1 << ARG_INPUT)  | (ARG_INT << 16) | 23;
  // argTypes[2] = 0;

  // printf("argTypes 0 - %d \n argTypes 1 - %d\n", argTypes[0], argTypes[1]);
  // int head[2];
  // head[0] = 0;
  // head[1] = RPC_TERMINATE;
  // int len = sizeof(head);

  // printf("The size of head is %d\n", len); 

  int sockfd, sockToClientfd;
  sockfd = BinderServer::getInstance()->startServer();


  sockToClientfd = wait_for_conn(sockfd);

  int a = 4;
  int b = 5;

  send(sockToClientfd, &a, sizeof(a), 0);
  send(sockToClientfd, &b, sizeof(b), 0);

  read_message(sockToClientfd);

  close(sockToClientfd);
  close(sockfd);

  return 0;

}

