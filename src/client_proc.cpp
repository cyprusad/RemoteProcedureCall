/*
 * client_proc.cpp
 * 
 * This is the client process
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
 // #include <sys/types.h>
 // #include <sys/wait.h>
 // #include <sys/unistd.h>
 // #include <unistd.h>
 // #include <netinet/in.h>
 // #include <arpa/inet.h>
 // #include <netdb.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h"
#include "utility.hpp"
#include "response_codes.h"

using namespace std;

// // get sockaddr, IPv4 or IPv6:
// void *get_in_addr(struct sockaddr *sa)
// {
//     if (sa->sa_family == AF_INET) {
//         return &(((struct sockaddr_in*)sa)->sin_addr);
//     }

//     return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }

// int call_sock(char hostname[], char port[]) {
//     int sockfd;
//     struct addrinfo hints, *servinfo, *p;
//     int rv;
//     char s[INET6_ADDRSTRLEN];

//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_UNSPEC;
//     hints.ai_socktype = SOCK_STREAM;

//     if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
//         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
//         return 1;
//     }

//     // loop through all the results and connect to the first we can
//     for(p = servinfo; p != NULL; p = p->ai_next) {
//         if ((sockfd = socket(p->ai_family, p->ai_socktype,
//                 p->ai_protocol)) == -1) {
//             perror("client: socket");
//             continue;
//         }

//         if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
//             perror("client: connect");
//             continue;
//         }
//         close(sockfd);

//         break;
//     }

//     if (p == NULL) {
//         fprintf(stderr, "client: failed to connect\n");
//         return 2;
//     }

//     inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
//             s, sizeof s);
//     printf("client: connecting to %s\n", s);

//     freeaddrinfo(servinfo); // all done with this structure

//     return sockfd;
// }

// // blocking - read head
// int* read_head(int sockfd) {
//   int head[2];
//   int nbytes;

//   nbytes = recv(sockfd, head, sizeof(head), 0);
//   // printf("read %d bytes\n", nbytes);
//   // printf("read len: %d \nread type: %d \n", head[0], head[1]);

//   return head;
// }


// //blocking send calls (block till full message is sent)
// int send_loc_request(int sockfd, char funcName[], int argTypes[], int sizeOfArgTypes) {
//   printf("send_loc_request :: ");
//   int len = (sizeof(char)*64) + sizeOfArgTypes*4; //funcName is is considered const size (max 64)
//   printf("SEND:\nSending total of %d bytes\n", len);
//   int head[2];
//   head[0] = len;
//   head[1] = RPC_LOC_REQUEST;

//   printf("The funcname is: %s\nThe argType array len is: %d\n", funcName, sizeOfArgTypes);

//   int bytesSent;

//   bytesSent = send(sockfd, head, sizeof(head), 0);

//   bytesSent = send(sockfd, funcName, (sizeof(char)*64), 0);

//   bytesSent = send(sockfd, argTypes, sizeOfArgTypes*4, 0);

//   return 0;
// }

// int send_terminate(int sockfd) {
//   printf("send_terminate :: \n");
//   int head[2];
//   head[0] = 0;
//   head[1] = RPC_TERMINATE;

//   int bytesSent = send(sockfd, head, sizeof(head), 0);
//   return 0; //error checking required
// }

// int send_execute(int binderSockFd, char* hostname, unsigned short port, int* argTypes, int sizeOfArgTypes, void** args, int sizeOfArgs) {
//   cout << "send_execute" << endl;
// }

class ClientProcess {
  private:
    static ClientProcess* singleton;
    int binderSockFd;
    char* BINDER_ADDRESS;
    char* BINDER_PORT;

  protected:
    ClientProcess() { // called only once in the ctr
      BINDER_ADDRESS = getenv("BINDER_ADDRESS");
      BINDER_PORT = getenv("BINDER_PORT");

      binderSockFd = call_sock(BINDER_ADDRESS, BINDER_PORT);
      locationForCall = NULL;
    }
  public:
    ServerPortCombo* locationForCall;

    static ClientProcess* getInstance() {
      if(singleton == 0) {
        singleton = new ClientProcess();
      }
      return singleton;
    }

    int getBinderSockFd(){
      return binderSockFd;
    }

    int read_message(int sockfd) {
      int resp;
      int* head = read_head(sockfd);
      int len = head[0];
      int type = head[1];

      switch(type) {
        case RPC_LOC_SUCCESS:
          resp = read_loc_success(sockfd); //could make use of ClientResp here
          break;
        case RPC_LOC_FAILURE:
          resp = read_loc_failure(sockfd);
          break;
        case RPC_EXECUTE_SUCCESS:
          resp = read_execute_success(sockfd, len);
          break;
        case RPC_EXECUTE_FAILURE:
          resp = read_execute_failure(sockfd);
          break;
        default:
          resp = INVALID_MESSAGE;
      }

      //TODO -- based on the response received, act.
      return 0;
    }

    int read_loc_success(int sockfd) {
      char hostname[sizeof(char)*128];
      unsigned short port;
      int nbytes;

      nbytes = recv(sockfd, hostname, (sizeof(char)*128), 0);

      nbytes = recv(sockfd, &port, sizeof(unsigned short), 0);

      cout << "LOC_SUCCESS :: " << "Server located has hostname: " << hostname << " and port: " << port << endl;

      // client stores this for every call and cleans it when execute message has been sent
      locationForCall = new ServerPortCombo(hostname, port);

      return 0;
    }

    int read_loc_failure(int sockfd) {
      int nbytes;
      int reasonCode;

      nbytes = recv(sockfd, &reasonCode, sizeof(int), 0);

      printf("LOC_FAILURE :: The reasonCode is: %d\n", reasonCode);
      return 0;
    }

    int read_execute_success(int sockfd, int len) {

    }

    int read_execute_failure(int sockfd) {
      int nbytes;
      int reasonCode;

      nbytes = recv(sockfd, &reasonCode, sizeof(int), 0);

      printf("EXECUTE_FAILURE :: The reasonCode is: %d\n", reasonCode);
      return 0;
    }

    // blocking call - wait on the binder to respond and then act accordingly
    int locationRequest(char* name, int* argTypes, int sizeOfArgTypes){
      int resp = send_loc_request(binderSockFd, name, argTypes, sizeOfArgTypes);

      // TODO The only bad thing that can happen here is that binder hangs up before data is sent

      return 0;
    }

    int execute(char* name, int* argTypes, int sizeOfArgTypes, void ** args, int sizeOfArgs) {
      //int sizeOfArgs = 0;
      char hostname[128];
      unsigned short port;

      if (locationForCall != NULL) {
        strcpy(hostname, locationForCall->name.c_str());
        port = locationForCall->port;

        cout << "execute :: " << "Sending execute to: " << hostname << ":" << port << endl;
        // this method is actually sent to the server not the binder
        int resp = send_execute(binderSockFd, hostname, port, argTypes, N_ELEMENTS(argTypes), args, sizeOfArgs);
        
        //free the server/port object
        delete locationForCall;
      }

      
    }

    int terminate() {
      int res = send_terminate(binderSockFd);
      return res;
    }

};

ClientProcess* ClientProcess::singleton = NULL;

// blocking call to binder and then server
int rpcCall(char* name, int* argTypes, void** args) {
  // locate server for me
  int binderSockFd = ClientProcess::getInstance()->getBinderSockFd();

  int resp = ClientProcess::getInstance()->locationRequest(name, argTypes, N_ELEMENTS(argTypes)); // fire the message to binder

  resp = ClientProcess::getInstance()->read_message(binderSockFd); // read either loc_success / failure

  //TODO if the resp was that the location lookup was unsuccessful, then return now


  resp = ClientProcess::getInstance()->execute(name, argTypes,N_ELEMENTS(argTypes), args, 0); // fire the message to server that was located

  resp = ClientProcess::getInstance()->read_message(binderSockFd);

  //TODO if the execution was successful then return 0;
  return 0;
}

int rpcTerminate() {
  //send the terminate message to the binder
  int res = ClientProcess::getInstance()->terminate();
  return res;
}

