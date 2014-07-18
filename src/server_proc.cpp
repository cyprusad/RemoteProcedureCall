#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h" 

using namespace std;

class ServerProcess {
  // server receives register_failure/success and execute
  private:
    int sockServerFd, sockBinderFd;
    
    // to be used by the server to connect to the binder  
    char* BINDER_ADDRESS;
    char* BINDER_PORT;

    // to be used by the clients to connect to the server
    char SERVER_ADDRESS[128];
    unsigned short SERVER_PORT;
    
    static ServerProcess* singleton;

  protected:
    ServerProcess() {

      //connect to the binder NOTE binder needs to be running
      BINDER_ADDRESS = getenv("BINDER_ADDRESS");
      BINDER_PORT = getenv("BINDER_PORT");
      sockBinderFd = call_sock(BINDER_ADDRESS, BINDER_PORT);

    } 
  
  public: 
    //singleton accessor
    static ServerProcess* getInstance() {
      if (singleton == 0) {
        singleton = new ServerProcess();
      }
      return singleton;
    }

    int getServerSockFd() {
      return sockServerFd;
    }

    int getBinderSockFd() {
      return sockBinderFd;
    }
    
    int startServer();

    int registerWithBinder(char* name, int* argTypes) {
      int res = send_register(sockBinderFd, SERVER_ADDRESS, SERVER_PORT, name, argTypes, N_ELEMENTS(argTypes));
      return res;
    } 

    int read_message(int sockfd) {
      int* head = read_head(sockfd); //TODO check if we need to clean this
      int len = head[0];
      int type = head[1];

      switch(type) {
        case RPC_TERMINATE:
          // add to queue of messages to be processed (graceful termination)
          terminate();
          break;
        case RPC_REGISTER_SUCCESS:
          read_reg_succ(sockfd);
          break;
        case RPC_REGISTER_FAILURE:
          read_reg_fail(sockfd);
          break;
        case RPC_EXECUTE:
          read_execute(sockfd, len);
          break;
        default:
          // invalid message type -- raise error of some sort
          invalid_message();
      }
    }

    int read_reg_succ(int sockfd) {
      int nbytes;
      int warningFlag;

      nbytes = recv(sockfd, &warningFlag, sizeof(int), 0);

      printf("The warningFlag is: %d\n", warningFlag);
      return 0;
    }

    int read_reg_fail(int sockfd) {
      int nbytes;
      int reasonCode;

      nbytes = recv(sockfd, &reasonCode, sizeof(int), 0);

      printf("The reasonCode is: %d\n", reasonCode);
      return 0;
    }

    int read_execute(int sockfd, int len) {
      
      
      return 0;
    }

    int terminate(); // TODO terminate server after verifying msg from binder

    int invalid_message() {
      printf("Invalid message\n");
      return 0; //or some warning
    }
};

ServerProcess* ServerProcess::singleton = NULL;

// TODO start server in background thread
int ServerProcess::startServer() {
  sockServerFd = setup_server("0");
  unsigned short* portPtr = &SERVER_PORT;
  
  if (addrAndPort(sockServerFd, SERVER_ADDRESS, portPtr) == 0) {
    printf("SERVER_ADDRESS %s\n", SERVER_ADDRESS);
    printf("SERVER_PORT    %hu\n", SERVER_PORT);
  } else {
    printf("warning[0]: server doesn't know it's hostname, possible that it didn't start properly\n");
  }

  return sockServerFd; //TODO check errors
}

//STATUS: Done
int rpcInit() { 
  ServerProcess::getInstance()->startServer();
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
  ServerProcess::getInstance()->registerWithBinder(name, argTypes); 
  //TODO check response and see if the registration failed
  

  //store skeleton in local DB

  return 0;
}

int rpcExecute() {
  //TODO - some kind of infinite accept loop; 
  //TODO - receive and process requests from client; 
  //TODO - multithreaded?
  int c = wait_for_conn(ServerProcess::getInstance()->getServerSockFd());

}

// This main is basically like what will be executed when rpcExecute is called
int main() {
  ServerProcess::getInstance()->startServer();
  //int c = wait_for_conn(ServerProcess::getInstance()->getServerSockFd());

  char herp[64] = "herp";
  int argTypes0[5];
  argTypes0[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16); 
  argTypes0[1] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16) | 23;
  argTypes0[2] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16);
  argTypes0[3] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 23;
  argTypes0[4] = 0;

  char derp[64] = "derp";
  int argTypes1[5];
  argTypes1[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16); 
  argTypes1[1] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16) | 23;
  argTypes1[2] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16);
  argTypes1[3] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 23;
  argTypes1[4] = 0;

  //send_register(ServerProcess::getInstance()->getBinderSockFd(), SERVER_ADDRESS, SERVER_PORT, herp, argTypes, N_ELEMENTS(argTypes));
  ServerProcess::getInstance()->registerWithBinder(herp, argTypes0);
  return 0;
}

