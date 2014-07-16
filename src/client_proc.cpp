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
    }
  public:
    static ClientProcess* getInstance() {
      if(singleton == 0) {
        singleton = new ClientProcess();
      }
      return singleton;
    }

    int getBinderSockFd(){
      return binderSockFd;
    }

    int locationRequest(){
      return 0;
    }

    int terminate() {
      int res = send_terminate(binderSockFd);
      return res;
    }

};

ClientProcess* ClientProcess::singleton = NULL;

int rpcCall(char * name, int * argTypes, void ** args) {
  // locate server for me


  // execute request to server
}

int rpcTerminate() {
  //send the terminate message to the binder
  int res = ClientProcess::getInstance()->terminate();
  return res;
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

  //send_terminate(binderSockFd);
  //send_register_failure(binderSockFd, 23);
  char func[64] = "saiprasad";
  int argTypes[5];
  argTypes[0] = 100;
  argTypes[1] = 200;
  argTypes[2] = 500;
  argTypes[3] = 600;
  argTypes[4] = 0;

  //printf("The size of func: %d\n and size of argTypes: %d\n", N_ELEMENTS(func), N_ELEMENTS(argTypes));
  send_loc_request(binderSockFd, func, argTypes, N_ELEMENTS(argTypes));

  close(binderSockFd);

}

