/*
 * binder.cpp
 * 
 * This is the binder process
 */
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <pthread.h> 

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h" 
#include "utility.h"
#include "response_codes.h"

using namespace std;

pthread_t b_threadid[NTHREADS]; // Thread pool
pthread_mutex_t b_lck;
int isTerminate = 0; // set to 1 when terminate is called


// search, look up and conflict resolution done here
// also make it thread safe
// NOTE  - so the singleton needs to be created before any threads are started - and after that
// access to the database should be in critical sections
class BinderDatabase {
  private:
    // for both vectors of funcs and servers, each index denotes matching servers and funcs
    vector<Func*> registeredFunctions;
    vector< list<ServerPortCombo*> > registeredServers; 
    
  public:
    vector<ServerPortCombo*> uniqueServers;

    BinderDatabase() {
      cout << "BinderDatabase :: " << "binder db ctr" << endl;
    }
    ~BinderDatabase() {
      cout << "~BinderDatabase :: " << "binder db destructor" << endl;

      // clear registered funcs
      for (vector<Func*>::iterator it = registeredFunctions.begin() ; it != registeredFunctions.end(); ++it) {
        delete *it;
      }
      registeredFunctions.clear();

      // clear unique servers
      for (vector<ServerPortCombo*>::iterator it = uniqueServers.begin() ; it != uniqueServers.end(); ++it) {
        delete *it;
      }
      uniqueServers.clear();

      // clear registered servers
      for (int i = 0; i < registeredServers.size(); i++) {
        registeredServers[i].clear();
      }
      registeredServers.clear();
    }
    // return index of registered function if present; -1 if not present
    //TODO if we are going to use this func for the client as well then do something about client/server array size
    int findRegisteredFunction(Func* func) { //used only internally by the database to register servers so far
      cout << "findRegisteredFunction :: " << endl;
      int respCode;
      if (registeredFunctions.size() == 0) {
        cout << "findRegisteredFunction :: " << " No funcs currently registered " << endl;
        return -1; // no function has been registered yet
      } else {
        cout << "findRegisteredFunction :: " << "There are " << registeredFunctions.size() << " funcs registered" << endl;
        for (int i = 0; i < registeredFunctions.size(); i++) {
          cout << "findRegisteredFunction :: " << "Name of server reg func " << func->name << endl;
          cout << "findRegisteredFunction :: " << "Name of func at index: " << i << " is: " << registeredFunctions[i]->name << endl;
          respCode = func->equals(registeredFunctions[i]);
          if (respCode >= 0) { // exact match or differ by array size for some arg
            cout << "findRegisteredFunction :: " << " Found func at index: " << i << endl;
            return i;
          }
        }
      }
      cout << "findRegisteredFunction :: " << "Func is not registered" << endl;
      return -1; // if we made it this far then we didn't find the function
    }

    // go through all the functions and find
    // round robin algorithm if multiple servers can service this client
    ClientResp* getServerPortComboForFunc(Func* func) {
      cout << "clientResp :: " << "Locating servers which serve the func requested " << endl;
      int index = findRegisteredFunction(func);
      ClientResp* res;
      if (index < 0) {
        cout << "clientResp :: " << "Function is not registered yet" << endl;
        res = new ClientResp(-1, NULL);
      } else {
        // round robin server
        cout << "clientResp :: " << "Function found in the database in the index " << index << endl;
        res = new ClientResp(1, findServer(index));
      }
      return res; //release this when sending the message back to client
    }

    // reference to server/port object is managed by the database
    ServerPortCombo* findServer(int index) {
      if (registeredServers[index].size() == 1) {
        cout << "findServer :: " << "Only one server registered for this func" << endl;
        cout << "findServer :: " << "Server selected is: " << registeredServers[index].front()->name << endl;
        return registeredServers[index].front();
      } else {
        cout << "findServer :: " << "Multiple servers registered for this func" << endl;
        ServerPortCombo* roundRobin = registeredServers[index].front();
        registeredServers[index].push_back(roundRobin);
        registeredServers[index].pop_front();

        cout << "findServer :: " << "Server selected is: " << roundRobin->name << endl;
        cout << "findServer :: " << "Next time this func will be served by " << registeredServers[index].front()->name << endl;
      }
    }

    ServerPortCombo* fetchUniqueServer(char hostname[], unsigned short port, int sock) {
      cout << "fetchUniqueServer :: " << endl;
      ServerPortCombo* temp = new ServerPortCombo(hostname, port, sock);
      if (uniqueServers.size() == 0) {
        cout << "fetchUniqueServer :: " << "No servers currently in the DB" << endl;
        uniqueServers.push_back(temp);
        return temp;
      } else {
        cout << "fetchUniqueServer :: " << uniqueServers.size() << " servers currently registered "<< endl;
        for (int i = 0; i < uniqueServers.size(); i++) {
          if (temp->equals(uniqueServers[i]) == 0) {
            delete temp;
            return uniqueServers[i];
          }
        }
      }
      return temp;
    }

    // check if the func already exists, in which case add to the existing registered servers for that index
    // NOTE that we need to add new funcs and respective server/ports in a lockstep fashion to get
    // the nice property that finding the index to func will give the index to the pool of servers
    // registered to serve that function
    int addFunc(Func* func, ServerPortCombo* server) {
      int resp;
      cout << "addFunc :: Func name recv: " << func->name << " @ " << server->name << ":" << server->port << endl;
      
      int findResult = findRegisteredFunction(func); // returns the index of function

      if (findResult == -1) { // seeing this function for the first time 
        cout << "addFunc :: " << "This is a NEW func" << endl;
        registeredFunctions.push_back(func);
        list<ServerPortCombo*> circularList;
        circularList.push_back(server);
        registeredServers.push_back(circularList); // adds in lockstep

        resp = BD_NEW_FUNC_REGISTERED;
        // TODO make some kind of assert to see if the addition did happen in lockstep
      } else { // also support function overloading - same server registers same function multiple times
        delete func; // exact same funciton already exists at index=findResult so delete the func created for addition

        for (list<ServerPortCombo*>::iterator it=registeredServers[findResult].begin(); it != registeredServers[findResult].end(); ++it) {
          if (server->equals(*it) == 0) {
            cout << "addFunc :: " << "This server has already registered this func" << endl;
            resp = BD_DUPL_FUNC_FOR_SERVER;
            return resp;
          }
        }
       
        registeredServers[findResult].push_back(server); // add new server registered for same function
        cout << "addFunc :: " << "The number of servers registered for same func are: " << registeredServers[findResult].size() << endl;
        resp = BD_NEW_SERVER_FOR_FUNC;
        
      }

      // sanity check
      cout << "addFunc :: " << "The size of reg funcs " << registeredFunctions.size() << endl;
      cout << "addFunc :: " << "Thse size of reg servers " << registeredServers.size() << endl;
      
      return resp; // (100,101,102)
    }
};


class BinderServer {
  // binder server receives loc_requests, register and terminate
  private:
    int sockSelfFd; // socket that accepts connections from client and servers
    
    // who am i
    char selfAddress[128];
    unsigned short selfPort;

    BinderDatabase* db;
    static BinderServer* singleton;

  protected:
    BinderServer() {
      db = new BinderDatabase();
    }

  public:

    static BinderServer* getInstance() {
      if (singleton == 0) {
        singleton = new BinderServer();
      }
      return singleton;
    }

    ~BinderServer() {
      delete db;
    }

    int startServer() {
      sockSelfFd = setup_server("0");

      unsigned short* portPtr = &selfPort;
      if (addrAndPort(sockSelfFd, selfAddress, portPtr) == 0) {
        printf("BINDER_ADDRESS %s\n", selfAddress);
        printf("BINDER_PORT    %hu\n", selfPort);
      } else {
        printf("warning[0]: binder doesn't know its hostname/port; possible that binder didn't start properly\n");
      }

      return sockSelfFd; //TODO error checking here
    }

    // blocking full message read and response - spawned on a different thread on accepting
    int read_message_and_respond(int sockfd) {
      int readResp = -1;
      int sendResp = -1;
      ClientResp* cResp = NULL;
      int* head = read_head(sockfd); // don't think we need to clean this up
      int len = head[0];
      int type = head[1];

      switch(type) {
        case RPC_TERMINATE:
          sendResp = terminate();
          break;
        case RPC_LOC_REQUEST:
          cResp = read_loc_request(sockfd, len);
          break;
        case RPC_REGISTER:
          readResp = read_register(sockfd, len);
          break;
        default:
          readResp = INVALID_MESSAGE;
      }

      // the read message was a RPC_LOC_REQUEST
      if (cResp != NULL) {
        if (cResp->respCode < 0) {
          sendResp = send_loc_failure(sockfd, cResp->respCode);
        } else {
          char server_id[128];
          std::strcpy(server_id, cResp->server->name.c_str());
          sendResp = send_loc_success(sockfd, server_id, cResp->server->port);
        }
        return sendResp; //return after sending message to client
      }

      if (readResp == BD_DUPL_FUNC_FOR_SERVER) {
        sendResp = send_register_failure(sockfd, readResp); // note that this socket is connected to server keep it alive
      }

      if (readResp == BD_NEW_FUNC_REGISTERED || readResp == BD_NEW_SERVER_FOR_FUNC) {
        sendResp = send_register_success(sockfd, readResp); // note that this socket is connected to server keep it alive
      }

      return sendResp;
    }

    ClientResp* read_loc_request(int sockfd, int len) {
      ClientResp* resp;
      int argTypesSize = (len - (sizeof(char)*64))/4;
      char funcName[sizeof(char)*64];
      int argTypes[argTypesSize];
      int nbytes;

      nbytes = recv(sockfd, funcName, (sizeof(char)*64), 0);

      nbytes = recv(sockfd, argTypes, (len - (sizeof(char)*64)), 0);

      printf("The func name read is: %s\nThe first elem of argTypes is: %d\n", funcName, argTypes[0]);


      // this should be in a critical section
      Func* queryFunc = new Func(funcName, argTypes, argTypesSize);

      //critical section -- get location of server
      pthread_mutex_lock (&b_lck);
      resp = db->getServerPortComboForFunc(queryFunc);
      pthread_mutex_unlock (&b_lck);

      delete queryFunc;

      return resp;
    }

    int read_register(int sockfd, int len) {
      ServerPortCombo* uniqueServer;
      Func* toAdd;
      printf("READ:\nReading a total of %d bytes\n", len);

      int argTypesSize = (len - (sizeof(char)*128) - sizeof(unsigned short) - (sizeof(char)*64))/4; // subtract the size of hostname, portnum, funcName
      printf("READ: argTypesSize = %d\n", argTypesSize);
      char server_identifier[sizeof(char)*128];
      char funcName[sizeof(char)*64];
      unsigned short portnum;
      int argTypes[argTypesSize];
      int nbytes;
      int respAddFunc;

      nbytes = recv(sockfd, server_identifier, (sizeof(char)*128), 0);

      nbytes = recv(sockfd, &portnum, sizeof(unsigned short), 0);

      nbytes = recv(sockfd, funcName, (sizeof(char)*64), 0);

      nbytes = recv(sockfd, argTypes, (argTypesSize*4), 0);

      printf("READ: The size of arg types %d\nThe server registered is: %s\nThe port of server is: %u\nThe func name is: %s\nThe first elem of argTypes is: %d\n",argTypesSize, server_identifier, portnum, funcName, argTypes[0]);

      pthread_mutex_lock (&b_lck);
      uniqueServer = db->fetchUniqueServer(server_identifier, portnum, sockfd);
      pthread_mutex_lock (&b_lck);

      toAdd = new Func(funcName, argTypes, argTypesSize);

      pthread_mutex_lock (&b_lck);
      respAddFunc = db->addFunc(toAdd, uniqueServer);
      pthread_mutex_unlock (&b_lck);
    
      //TODO:
      // reasons to raise an error/warning:
      // - while reading data from server, the socket closed for some reason
      // - the registereing of func fails
      // - the binder crashes or something else bad happens
      return respAddFunc; //(100/101/102 or something bad happened with the socket reading)
    }

    int terminate(){
      int sockfd, sendResp;
      for (int i = 0; i < db->uniqueServers.size(); i++) {
        sockfd = db->uniqueServers[i]->sockfd; // this probably a bad idea
        sendResp = send_terminate(sockfd);
      }

      pthread_mutex_lock (&b_lck);
      isTerminate = 1;
      pthread_mutex_unlock (&b_lck);

      // if the send happened successfully then 
      return BS_SUCC_SEND_TERM_SERVER ;
    }

};


BinderServer* BinderServer::singleton = NULL;


void *threadworker(void* arg)
{
  printf("TID:0x%x has been spawned\n", pthread_self());

  int sockfd = (int)arg; 

  cout << "threadworker :: " << "The sock passed to me is " << sockfd << endl;

  //TODO The resp should uniquely tell if the read message was register/location and what happened 
  int resp = BinderServer::getInstance()->read_message_and_respond(sockfd); // Blocks until there is something to be read in the socket

  while (resp == BS_SUCC_SEND_SERVER) { // check if server socket .. or not closed
    resp = BinderServer::getInstance()->read_message_and_respond(sockfd); // keep reading register calls from the server
  }

  // if we are here, it means that this was a client socket.. so close it
  printf("TID:0x%x is done processing: either client is served or the server hung up\n", pthread_self());
  close(sockfd);
  pthread_exit(0);
}


int main() {
  int sockSelfFd, sockIncomingFd;
  int terminate;

  // This will initialize the singleton server and database and we are still in single thread mode
  sockSelfFd = BinderServer::getInstance()->startServer();

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

     pthread_mutex_lock (&b_lck);
     terminate = isTerminate;
     pthread_mutex_unlock (&b_lck);

     if (terminate != 0) {
      break;
     }

     pthread_create(&b_threadid[i++], &attr, &threadworker, (void *) sockIncomingFd);
     sleep(0); // Giving threads some CPU time
  }

  return 0;

}

