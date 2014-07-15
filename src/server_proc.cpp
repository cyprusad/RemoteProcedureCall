#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rpc.h"
#include "sck_stream.h" 

using namespace std;

ServerProcess* ServerProcess::singleton = NULL;

class ServerProcess {
  private:
    int sockServer, sockBinder;
    char* BINDER_ADDRESS;
    char* BINDER_PORT;
    
    // singleton instance
    static ServerProcess* singleton;

  protected:
    ServerProcess(); 
  
  public: 
    //singleton accessor
    static ServerProcess* getInstance() {
      if (singleton == 0) {
        singleton = new ServerProcess();
      }
      return singleton;
    }

    int getServerSockFd() {
      return sockServer;
    }
    
    int startServer(); 

    int connectToBinder();
};

int ServerProcess::startServer() {
  sockServer = setup_server("0", 0); // server is calling so no need to print the addr/port
  return sockServer; 
}

int ServerProcess::connectToBinder() {
  BINDER_ADDRESS = getenv("BINDER_ADDRESS");
  BINDER_PORT = getenv("BINDER_PORT");
  int s = call_sock(BINDER_ADDRESS, BINDER_PORT);
}

int rpcInit() {
  // bind to the server 
}

int main() {
  //ServerProcess* server = ServerProcess::getInstance();
  //ServerProcess* server = new ServerProcess();
  ServerProcess::getInstance()->startServer();
  int c = wait_for_conn(ServerProcess::getInstance()->getServerSockFd());

  return 0;
}

