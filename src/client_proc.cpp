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

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h"
#include "utility.h"
#include "response_codes.h"

using namespace std;

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

    int execute(char * name, int * argTypes, void ** args) {
      int sizeOfArgs = 0;
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


  //resp = ClientProcess::getInstance()->execute(char * name, int * argTypes, void ** args); // fire the message to server that was located

  //resp = ClientProcess::getInstance()->read_message(binderSockFd);

  //TODO if the execution was successful then return 0;
  return 0;
}

int rpcTerminate() {
  //send the terminate message to the binder
  int res = ClientProcess::getInstance()->terminate();
  return res;
}

// int main() {
//   int binderSockFd = ClientProcess::getInstance()->getBinderSockFd();
 
//   char name[64] = "derp";
//   int argTypes1[5];
//   argTypes1[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16); 
//   argTypes1[1] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16) | 23;
//   argTypes1[2] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16);
//   argTypes1[3] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 23;
//   argTypes1[4] = 0;

//   int resp = ClientProcess::getInstance()->locationRequest(name, argTypes1, N_ELEMENTS(argTypes1)); // fire the message to binder

//   resp = ClientProcess::getInstance()->read_message(binderSockFd); // read either loc_success / failure

//   //printf("The size of func: %d\n and size of argTypes: %d\n", N_ELEMENTS(func), N_ELEMENTS(argTypes));
//   //rpcCall()

//   //close(ClientProcess::getInstance()->getBinderSockFd()); // TODO I don't think we ever close conn to binder -- perhaps in binder
//   return 0;
// }

