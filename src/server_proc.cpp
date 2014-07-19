#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <stdint.h>

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h"
#include "utility.hpp" 
#include "response_codes.h"

using namespace std;

pthread_t s_threadid[NTHREADS]; // Thread pool
pthread_mutex_t s_lck;
int isTerminate = 0; // set to 1 when terminate is called

class ServerDatabase {
  public:
    vector<Func*> functions;
    vector<int (*)(int *, void **)> skeletons;

    ServerDatabase() {
      cout << "ServerDatabase :: " << "server db ctr" << endl;
    }
    ~ServerDatabase() {
      cout << "~ServerDatabase :: " << "server db destructor" << endl;

      // clear registered funcs
      for (vector<Func*>::iterator it = functions.begin() ; it != functions.end(); ++it) {
        delete *it;
      }
      functions.clear();

      // clear skeletons
      skeletons.clear();
    }

    // return index of registered function if present; -1 if not present
    int findRegisteredFunction(Func* func) { //used only internally by the database to register servers so far
      cout << "findRegisteredFunction :: " << endl;
      int respCode;
      if (functions.size() == 0) {
        cout << "findRegisteredFunction :: " << " No funcs currently registered locally " << endl;
        return -1; // no function has been registered yet
      } else {
        cout << "findRegisteredFunction :: " << "There are " << functions.size() << " funcs registered locally" << endl;
        for (int i = 0; i < functions.size(); i++) {
          cout << "findRegisteredFunction :: " << "Name of server reg func " << func->name << endl;
          cout << "findRegisteredFunction :: " << "Name of func at index: " << i << " is: " << functions[i]->name << endl;
          respCode = func->equals(functions[i]);
          if (respCode >= 0) { // exact match or differ by array size for some arg
            cout << "findRegisteredFunction :: " << " Found func at index: " << i << endl;
            return i;
          }
        }
      }
      cout << "findRegisteredFunction :: " << "Func is not registered" << endl;
      return -1; // if we made it this far then we didn't find the function
    }

    int addToDB(Func* func, skeleton f) {
      int index = findRegisteredFunction(func);
      if (index < 0) {
        functions.push_back(func);
        skeletons.push_back(f);
      } else {
        delete func; // free the func pointer created
        skeletons[index] = f; // replace the skeleton
      }
      return 0;
    }

    // Assume that when a client asksl skeleton is always there
    skeleton getSkeletonForFunc(Func* func) {
      int index = findRegisteredFunction(func);
      return skeletons[index];
    }
};

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

      db = new ServerDatabase();
      stillRegistering = 1;
    } 
  
  public: 
    ServerDatabase* db;
    //singleton accessor

    int stillRegistering;

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

    ServerDatabase* getDB(){
      return db;
    }
    int startServer();

    int registerWithBinder(char* name, int* argTypes, int sizeOfArgTypes) {
      int res = send_register(sockBinderFd, SERVER_ADDRESS, SERVER_PORT, name, argTypes, sizeOfArgTypes);
      
      res = read_message(sockBinderFd); //register success or fail
    } 

    int read_message(int sockfd) {
      int* head = read_head(sockfd); //TODO check if we need to clean this
      int len = head[0];
      int type = head[1];
      int readResp;

      switch(type) {
        case RPC_TERMINATE:
          // add to queue of messages to be processed (graceful termination)
          terminate();
          break;
        case RPC_REGISTER_SUCCESS:
          readResp = read_reg_succ(sockfd);
          break;
        case RPC_REGISTER_FAILURE:
          readResp = read_reg_fail(sockfd);
          break;
        case RPC_EXECUTE:
          readResp = read_execute(sockfd, len);
          break;
        default:
          // invalid message type -- raise error of some sort
          readResp = INVALID_MESSAGE;
      }

      return 0;
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

    int terminate() {
      printf("Server terminate\n");
      return 0;
    }

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
  cout << "rpcInit :: " << endl; 
  ServerProcess::getInstance()->startServer();
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
  Func* toAdd;
  int resp = ServerProcess::getInstance()->registerWithBinder(name, argTypes, N_ELEMENTS(argTypes)); 
  //TODO check response and see if the registration failed
  
  //store skeleton in local DB
  toAdd = new Func(name, argTypes, N_ELEMENTS(argTypes));
  resp = ServerProcess::getInstance()->getDB()->addToDB(toAdd, f);
  return 0;
}

void *serverWorker(void* arg)
{
  printf("TID:0x%x has been spawned\n", pthread_self());

  intptr_t sockfd = (intptr_t)arg; 

  cout << "threadworker :: " << "The sock passed to me is " << sockfd << endl;

  //TODO The resp should uniquely tell if the read message was register/location and what happened 
  int resp = ServerProcess::getInstance()->read_message(sockfd); // Blocks until there is something to be read in the socket

   // if we are here, it means that this was a client socket.. so close it
  printf("TID:0x%x is done processing or client hung up\n", pthread_self());
  close(sockfd);
  pthread_exit(0);
}

int rpcExecute() {
  int sockSelfFd, sockIncomingFd;
  int terminate;

  // This will initialize the singleton server and database and we are still in single thread mode
  sockSelfFd = ServerProcess::getInstance()->getServerSockFd();

  pthread_attr_t attr; // Thread attribute
  int i; // Thread iterator

  pthread_attr_init(&attr); // Creating thread attributes
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO); // FIFO scheduling for threads 
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // Don't want threads (particualrly main)
  
                                                                
  i = 0;
  while (1) {
     if (i == NTHREADS) // So that we don't access a thread out of bounds of the thread pool
     {
       i = 0;
     }

     sockIncomingFd = wait_for_conn(sockSelfFd);

     cout << "main thread :: " << "The sock accepted is " << sockIncomingFd << endl;

     pthread_mutex_lock (&s_lck);
     terminate = isTerminate;
     pthread_mutex_unlock (&s_lck);

     if (terminate != 0) {
      break;
     }

     pthread_create(&s_threadid[i++], &attr, &serverWorker, (void *) sockIncomingFd);
     sleep(0); // Giving threads some CPU time
  }

  return 0;

}

