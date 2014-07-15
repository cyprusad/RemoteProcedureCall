#include <stdio.h>

#include "rpc.h"
#include "sck_stream.h" 

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

  unsigned short portnum = 34548;

  char port[] = "50193";

  //s = call_socket(hostname, portnum);

  s = call_sock(hostname, port);

  printf("Connection successful to socket? %d \n", s);

}

