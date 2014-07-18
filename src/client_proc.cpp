/*
 * client_proc.cpp
 * 
 * This is the client process
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h"

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

    int invalid_message() {
      printf("Invalid message\n");
      return 0; //or some warning
    }

    int read_message(int sockfd) {
      int resp;
      int* head = read_head(sockfd);
      int len = head[0];
      int type = head[1];

      switch(type) {
        case RPC_LOC_SUCCESS:
          resp = read_loc_success(sockfd);
        case RPC_LOC_FAILURE:
          resp = read_loc_failure(sockfd);
        case RPC_EXECUTE_SUCCESS:
          resp = read_execute_success(sockfd, len);
        case RPC_EXECUTE_FAILURE:
          resp = read_execute_failure(sockfd);
        default:
          resp = invalid_message();
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

      //TODO send the execute method to the server

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
    int locationRequest(char * name, int * argTypes){
      int resp = send_loc_request(binderSockFd, name, argTypes, N_ELEMENTS(argTypes));
      // TODO depending on the response -- do something

      //receive a message (either location fail or success)
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
  int resp = ClientProcess::getInstance()->locationRequest(name, argTypes);
  //TODO depending on the resp, either return or make the execute call to the server that we


  // execute request to server

}

int rpcTerminate() {
  //send the terminate message to the binder
  int res = ClientProcess::getInstance()->terminate();
  return res;
}

int main() {
  // int binderSockFd;

  // int a,b, numbytes;
  // char buf[MAXDATASIZE];

  // BINDER_ADDRESS = getenv("BINDER_ADDRESS");
  // BINDER_PORT = getenv("BINDER_PORT");

  // printf("addr = %s \n port = %s \n ", BINDER_ADDRESS, BINDER_PORT);

  // binderSockFd = call_sock(BINDER_ADDRESS, BINDER_PORT);

  // numbytes = recv(binderSockFd, &a, sizeof(a), 0);
  // printf("read %d bytes\n", numbytes);
  // printf("read a: %d \n", a);

  // numbytes = recv(binderSockFd, &b, sizeof(b), 0);
  // printf("read %d bytes\n", numbytes);
  // printf("read b: %d \n", b);

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
  //rpcCall()

  close(ClientProcess::getInstance()->getBinderSockFd()); // TODO I don't think we ever close conn to binder -- perhaps in binder

}

