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
      int res = send_register(sockBinderFd, SERVER_ADDRESS, SERVER_PORT, name, argTypes);
      return res;
    } 

    int terminate(); // TODO terminate server after verifying msg from binder
};

ServerProcess* ServerProcess::singleton = NULL;

// TODO start server in background thread
int ServerProcess::startServer() {
  sockServerFd = setup_server("0"); //TODO infinite loop listening to client

  unsigned short* portPtr = &SERVER_PORT;
  if (addrAndPort(sockServerFd, SERVER_ADDRESS, portPtr) == 0) {
    printf("SERVER_ADDRESS %s\n", SERVER_ADDRESS);
    printf("SERVER_PORT    %hu\n", SERVER_PORT);
  } else {
    printf("I don't know who I am");
  }

  return sockServerFd; 
}

int rpcInit() { 
  ServerProcess::getInstance()->startServer();
  //TODO open connection to binder
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
  ServerProcess::getInstance()->registerWithBinder(name, argTypes);
  //store skeleton in local DB

  return 0;
}

int rpcExecute() {
  //TODO - some kind of infinite accept loop; 
  //TODO - receive and process requests from client; 
  //TODO - multithreaded?
  int c = wait_for_conn(ServerProcess::getInstance()->getServerSockFd());

}

int main() {
  ServerProcess::getInstance()->startServer();
  int c = wait_for_conn(ServerProcess::getInstance()->getServerSockFd());

  return 0;
}

