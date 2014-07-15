#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h> 

#include "rpc.h"
#include "sck_stream.h"

#define MAXDATASIZE 64 

using namespace std;

int rpcCall(char * name, int * argTypes, void ** args) {
  // locate server for me
  // execute request to server
}

int rpcTerminate() {
  //send the terminate message to the binder
}

int main() {
  int s;

  char hostname[] = "Sai-Warangs-MacBook-Pro.local";

  char port[] = "50331";

  int a,b, numbytes;
  char buf[MAXDATASIZE];


  //s = call_socket(hostname, portnum);

  s = call_sock(hostname, port);

  numbytes = recv(s, &a, sizeof(a), 0);
  printf("read %d bytes\n", numbytes);
  printf("read a: %d \n", a);

  numbytes = recv(s, &b, sizeof(b), 0);
  printf("read %d bytes\n", numbytes);
  printf("read b: %d \n", b);

  close(s);

}

