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
  // binder server receives loc_requests, register and terminate
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

    // blocking full message read - spawned on a different thread on accepting
    int read_message(int sockfd) {
      int* head = read_head(sockfd); // don't think we need to clean this up
      int len = head[0];
      int type = head[1];

      switch(type) {
        case RPC_TERMINATE:
          // add to queue of messages to be processed (graceful termination)
          terminate();
          break;
        case RPC_LOC_REQUEST:
          // read additional message
          read_loc_request(sockfd, len);
          // add to queue of messages to be processed
          break;
        case RPC_REGISTER:
          // read additional content of the message
          read_register(sockfd, len);
          // add to queue of messages to be processed
          break;
        default:
          // invalid message type -- raise error of some sort
          invalid_method(sockfd);
      }

      return 0;
    }

    int read_loc_request(int sockfd, int len) {
      int argTypesSize = (len - 64)/4;
      char funcName[64];
      int argTypes[argTypesSize];
      int nbytes;

      nbytes = recv(sockfd, funcName, 64, 0);

      nbytes = recv(sockfd, argTypes, (len - 64), 0);

      printf("The func name read is: %s\nThe last elem of argTypes is: %d\n", funcName, argTypes[4]);

      return 0;
    }

    int read_register(int sockfd, int len) {
      int argTypesSize = (len - 128 - 2 - 64)/4; // subtract the size of hostname, portnum, funcName
      char server_identifier[128];
      char funcName[64];
      unsigned int portnum;
      int argTypes[argTypesSize];
      int nbytes;

      nbytes = recv(sockfd, server_identifier, 128, 0);

      nbytes = recv(sockfd, &portnum, 2, 0);

      nbytes = recv(sockfd, funcName, 64, 0);

      nbytes = recv(sockfd, argTypes, (len - 128 - 2 - 64)/4, 0);

      printf("READ: The server registered is: %s\nThe port of server is: %huThe func name is: %s\nThe first elem of argTypes is: %d\n", server_identifier, portnum, funcName, argTypes[0]);
      
      return 0;
    }

    int terminate(){
      // terminate all the servers

      // terminate self - shut off the sockets and clean up memory used
      return 0;
    }

    int invalid_method(int sockfd) {
      printf("Invalid method\n");
      return 0; //or some warning
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

  // int* head = read_head(sockToClientfd);
  // int len = head[0];
  // int type = head[1];
  // printf("Received message (head).. len=%d and type=%d\n", len, type);

  BinderServer::getInstance()->read_message(sockToClientfd);

  close(sockToClientfd);
  close(sockfd);

  return 0;

}

