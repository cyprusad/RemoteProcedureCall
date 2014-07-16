#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h" 

using namespace std;

ServerProcess* ServerProcess::singleton = NULL;

class ServerProcess {
  private:
    int sockServerFd, sockBinderFd;
    char* BINDER_ADDRESS;
    char* BINDER_PORT;
    
    static ServerProcess* singleton;

  protected:
    ServerProcess() {

      // connect to the binder
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

    int terminate(); // TODO terminate server after verifying msg from binder
};

// TODO start server in background thread ??
int ServerProcess::startServer() {
  sockServer = setup_server("0", 0); 
  return sockServer; 
}

int rpcInit() { 
  ServerProcess::getInstance()->startServer();
  // TODO find out who our hostname and port number to store in our internal db and send to binder 
}

int rpcRegister(char* name, int* argTypes, skeleton f);

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

