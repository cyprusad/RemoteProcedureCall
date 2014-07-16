#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h"

#define MAXDATASIZE 64 

using namespace std;

char* BINDER_ADDRESS;
char* BINDER_PORT;

int rpcCall(char * name, int * argTypes, void ** args) {
  // locate server for me
  // execute request to server
}

int rpcTerminate() {
  //send the terminate message to the binder

  //send_terminate(binderSockFd);

}

int main() {
  int binderSockFd;

  int a,b, numbytes;
  char buf[MAXDATASIZE];

  BINDER_ADDRESS = getenv("BINDER_ADDRESS");
  BINDER_PORT = getenv("BINDER_PORT");

  printf("addr = %s \n port = %s \n ", BINDER_ADDRESS, BINDER_PORT);

  binderSockFd = call_sock(BINDER_ADDRESS, BINDER_PORT);

  numbytes = recv(binderSockFd, &a, sizeof(a), 0);
  printf("read %d bytes\n", numbytes);
  printf("read a: %d \n", a);

  numbytes = recv(binderSockFd, &b, sizeof(b), 0);
  printf("read %d bytes\n", numbytes);
  printf("read b: %d \n", b);

  // int head[2];
  // head[0] = 0;
  // head[1] = RPC_TERMINATE;
  // int len = sizeof(head);

  // send(binderSockFd, head, sizeof(head), 0);


  // int bytesSent = send(binderSockFd, &head, len, 0);

  send_terminate(binderSockFd);

  close(binderSockFd);

}

