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
#include <vector>
#include <list>
#include <string>
#include <iostream> 

#include "rpc.h"
#include "sck_stream.h"
#include "message_protocol.h" 
#include "utility.h"

using namespace std;

// TODO move to separate header

// // always unique
// class ServerPortCombo {
//   public:
//     string name;
//     unsigned short port;

//     ServerPortCombo(char* n, unsigned int p) {
//       name.assign(n);
//       port = p;
//     }

//     ~ServerPortCombo() {
//       cout << "~ServerPortCombo :: " << "server/port destructor" << endl;
//     }

//     int equals(ServerPortCombo* other) {
//       if (name.compare(other->name) != 0) {
//         return -1;
//       } 
//       if (port != other->port) {
//         return -1;
//       }
//       return 0; // if you made it here, then they're both equal
//     }
// };

// class ClientResp {
//   public:
//     int respCode;
//     ServerPortCombo* server;

//     // -1 response would mean that no such function is registered with the binder; anything else will give the server/port
//     ClientResp(int res, ServerPortCombo* s) {
//       respCode = res;
//       server = s;
//     }

//     ~ClientResp() {
//       cout << "~ClientResp :: " << "client resp destructor" << endl;
//     }
// };

// class Arg {
//   public:
//     bool input;
//     bool output;
//     int type;
//     int arrSize;

//     Arg(bool in, bool out, int t, int arr_size) {
//       input = in;
//       output = out;
//       type = t;
//       arrSize = arr_size;
//     }

//     ~Arg() {
//       cout << "~Arg :: " << "arg destructor -- bottom of the stack" << endl;
//     }

//     // just checks the equality of argument signatures
//     int equals(Arg* other) {
//       if (input != other->input) {
//         return -1;
//       } else if (output != other->output) {
//         return -1;
//       } else if (type != other->type) {
//         return -1;
//       } else { // checked first three members, only array sizes remain
//         if (arrSize == 0) { // client method is a scalar
//           if (other->arrSize > 0) {
//             return -1; // registered method has array
//           } 
//         } else if(other->arrSize == 0) {
//           if (arrSize > 0) {
//             return -1; // we are an array but the registered method is a scalar
//           } 
//         } else {
//           if (arrSize > other->arrSize) {
//             return 2; // some kind of warning saying that the client array is greater than the max the server would like to allocate
//           } 
//         }
//       }
//       return 0; // if you made it this far, you are okay
//     }

//     // ensure that the arg is not zero!
//     static Arg* parseArg(int* arg) {
//       return new Arg((*arg & INPUT_MASK) >> ARG_INPUT, 
//                      (*arg & OUTPUT_MASK) >> ARG_OUTPUT, 
//                      (*arg & TYPE_MASK) >> 16,
//                      (*arg & ARRAY_MASK));
//     }
// }; 

// class Func {
//   public:
//     string name;
//     vector<Arg*> arguments;

//     Func(char* funcName, int argTypes[], int sizeOfArgTypes) {
//       name.assign(funcName);
//       arguments.reserve(sizeOfArgTypes - 1);

//       int* iterator = argTypes;
//       while(*iterator != 0) {
//         arguments.push_back(Arg::parseArg(iterator));
//         iterator++; 
//       }
//     }

//     ~Func() {
//       cout << "~Func :: " << "Func destructor for funcName = " << name << endl;
//       for (vector<Arg*>::iterator it = arguments.begin() ; it != arguments.end(); ++it) {
//         delete *it;
//       }
//       arguments.clear();
//     }

//     int equals(Func* other) {
//       int warningFlag = 0;
//       if (name.compare(other->name) != 0) {
//         return -1;
//       } 
//       if (arguments.size() != other->arguments.size()) {
//         return -1;
//       } else {
//         for (int i = 0; i < arguments.size(); i++) {
//           if (arguments[i]->equals(other->arguments[i]) < 0) { // one of the arguments don't match 
//             return -1;
//           } else if (arguments[i]->equals(other->arguments[i]) > 0) {
//             warningFlag = arguments[i]->equals(other->arguments[i]); // generated warning flag
//           }
//         }
//         if (warningFlag != 0) {
//           return warningFlag; // all the arguments and func names match, but client is requesting larger array than server wants to allocate
//         }
//       }
//       return 0;
//     }
// };

// search, look up and conflict resolution done here
// also make it thread safe
// NOTE  - so the singleton needs to be created before any threads are started - and after that
// access to the database should be in critical sections
class BinderDatabase {
  private:
    // for both vectors of funcs and servers, each index denotes matching servers and funcs
    vector<Func*> registeredFunctions;
    vector< list<ServerPortCombo*> > registeredServers; 
    
    static BinderDatabase* singleton;  

  public:
    static BinderDatabase* getInstance() {
      if (singleton == 0) {
        singleton = new BinderDatabase();
      }
      return singleton;
    }

    ~BinderDatabase() {
      cout << "~BinderDatabase :: " << "binder db destructor" << endl;

      // clear registered funcs
      for (vector<Func*>::iterator it = registeredFunctions.begin() ; it != registeredFunctions.end(); ++it) {
        delete *it;
      }
      registeredFunctions.clear();

      // clear registered servers
      for (int i = 0; i < registeredServers.size(); i++) {
        for (list<ServerPortCombo*>::iterator it = registeredServers[i].begin() ; it != registeredServers[i].end(); ++it) {
          delete *it;
        }
        registeredServers[i].clear();
      }
      registeredServers.clear();
    }
    // return index of registered function if present; -1 if not present
    //TODO if we are going to use this func for the client as well then do something about client/server array size
    int findRegisteredFunction(Func* func) { //used only internally by the database to register servers so far
      int respCode;
      if (registeredFunctions.size() == 0) {
        return -1; // no function has been registered yet
      } else {
        for (int i = 0; i < registeredFunctions.size(); i++) {
          respCode = func->equals(registeredFunctions[i]);
          if (respCode == 0 || respCode == 2) { // exact match or differ by array size for some arg
            return i;
          }
        }
      }
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
      return res;
    }

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

    // check if the func already exists, in which case add to the existing registered servers for that index
    // NOTE that we need to add new funcs and respective server/ports in a lockstep fashion to get
    // the nice property that finding the index to func will give the index to the pool of servers
    // registered to serve that function
    int addFunc(Func* func, ServerPortCombo* server) {
      int alreadyPresent = 0; // only gets set when same server tries to register twice
      cout << "addFunc :: Func name recv: " << func->name << endl << " Server name recv: " << server->name << endl;
      int findResult = findRegisteredFunction(func);
      // NOTE -2 is reserved for case when client wants more space than server but otherwise same
      if (findResult == -1) { // seeing this function for the first time 
        cout << "addFunc :: " << "This is a NEW func" << endl;
        registeredFunctions.push_back(func);
        list<ServerPortCombo*> circularList;
        circularList.push_back(server);
        registeredServers.push_back(circularList); // adds in lockstep

        // TODO make some kind of assert to see if the addition did happen in lockstep
      } else { // also support function overloading - same server registers same function multiple times

        for (list<ServerPortCombo*>::iterator it=registeredServers[findResult].begin(); it != registeredServers[findResult].end(); ++it) {
          if (server->equals(*it) == 0) {
            alreadyPresent = 1;
            cout << "addFunc :: " << "This server has already registered this func" << endl;
            break; // server is already registered with binder for this function (doing it twice)
          }
        }
     
        if (alreadyPresent == 0) { //same server is NOT registered for this func
          registeredServers[findResult].push_back(server); // add new server registered for same function
          cout << "addFunc :: " << "The number of servers registered for same func are: " << registeredServers[findResult].size() << endl;
        }
      }

      cout << "addFunc :: " << "The size of reg funcs " << registeredFunctions.size() << endl;
      cout << "addFunc :: " << "Thse size of reg servers " << registeredServers.size() << endl;
      return alreadyPresent; // this can be used as a warning code/error code for the server trying to register twice for same func
    }
};

BinderDatabase* BinderDatabase::singleton = NULL;

class BinderServer {
  // binder server receives loc_requests, register and terminate
  private:
    int sockSelfFd; // socket that accepts connections from client and servers
    
    // who am i
    char selfAddress[128];
    unsigned short selfPort;

    static BinderServer* singleton;

  protected:
    //BinderServer();

  public:

    static BinderServer* getInstance() {
      if (singleton == 0) {
        singleton = new BinderServer;
      }
      return singleton;
    }

    ~BinderServer() {
      delete BinderDatabase::getInstance();
    }

    int startServer() {
      sockSelfFd = setup_server("0");

      unsigned short* portPtr = &selfPort;
      if (addrAndPort(sockSelfFd, selfAddress, portPtr) == 0) {
        printf("BINDER_ADDRESS %s\n", selfAddress);
        printf("BINDER_PORT    %hu\n", selfPort);
      } else {
        printf("warning[0]: binder doesn't know its hostname/port; possible that binder didn't start properly");
      }

      return sockSelfFd; //TODO error checking here
    }

    // blocking full message read - spawned on a different thread on accepting
    int read_message(int sockfd) {
      int resp;
      int* head = read_head(sockfd); // don't think we need to clean this up
      int len = head[0];
      int type = head[1];

      switch(type) {
        case RPC_TERMINATE:
          // add to queue of messages to be processed (graceful termination)
          resp = terminate();
          break;
        case RPC_LOC_REQUEST:
          resp = read_loc_request(sockfd, len);
          //based on resp either exit/send a adequate response back

          // add to queue of messages to be processed
          break;
        case RPC_REGISTER:
          resp = read_register(sockfd, len);
          //based on resp either exit/send a adequate response back

          // add to queue of messages to be processed
          break;
        default:
          // invalid message type -- raise error of some sort
          resp = invalid_message();
      }

      // test destructor
      // cout << "readMessage :: " << "Calling DB destructor" << endl;
      // delete BinderDatabase::getInstance();

      //TODO -- based on the response received, act.

      return 0;
    }

    int read_loc_request(int sockfd, int len) {
      int argTypesSize = (len - (sizeof(char)*64))/4;
      char funcName[sizeof(char)*64];
      int argTypes[argTypesSize];
      int nbytes;

      nbytes = recv(sockfd, funcName, (sizeof(char)*64), 0);

      nbytes = recv(sockfd, argTypes, (len - (sizeof(char)*64)), 0);

      printf("The func name read is: %s\nThe last elem of argTypes is: %d\n", funcName, argTypes[4]);


      // this should be in a critical section
      ClientResp* resp = BinderDatabase::getInstance()->getServerPortComboForFunc(new Func(funcName, argTypes, argTypesSize));

      if (resp->respCode < 0) {
        // we DONT have a server that will process the request for us

        //send location failure to the client
      }

      // TODO call destructor of ClientResp when done sending the LOC_SUCCESS/LOC_FAILURE message
      return 0;
    }

    int read_register(int sockfd, int len) {
      printf("READ:\nReading a total of %d bytes\n", len);

      int argTypesSize = (len - (sizeof(char)*128) - sizeof(unsigned short) - (sizeof(char)*64))/4; // subtract the size of hostname, portnum, funcName
      char server_identifier[sizeof(char)*128];
      char funcName[sizeof(char)*64];
      unsigned short portnum;
      int argTypes[argTypesSize];
      int nbytes;

      nbytes = recv(sockfd, server_identifier, (sizeof(char)*128), 0);

      nbytes = recv(sockfd, &portnum, sizeof(unsigned short), 0);

      nbytes = recv(sockfd, funcName, (sizeof(char)*64), 0);

      nbytes = recv(sockfd, argTypes, (argTypesSize*4), 0);

      printf("The server registered is: %s\nThe port of server is: %u\nThe func name is: %s\nThe first elem of argTypes is: %d\n", server_identifier, portnum, funcName, argTypes[0]);

      //Register with the database
      int resp = BinderDatabase::getInstance()->addFunc(new Func(funcName, argTypes, argTypesSize),
                                                        new ServerPortCombo(server_identifier, portnum));

      if (resp < 0) {
        // registration was NOT successful, send registration failure
      }

      //TODO:
      // reasons to raise an error/warning:
      // - while reading data from server, the socket closed for some reason
      // - the registereing of func fails
      // - the binder crashes or something else bad happens
      return 0;
    }

    int terminate(){
      // terminate all the servers

      // terminate self - shut off the sockets and clean up memory used
      return 0;
    }

    int invalid_message() {
      printf("Invalid message\n");
      return 0; //or some warning
    }


};


BinderServer* BinderServer::singleton = NULL;

// create a custom mask on the 32 bit integer
int mask(int start, int finish) {
  int res = 0; 
  for (int i = start; i < finish; i++) { 
    res = res | (1 << i); 
  } 
  return res;
}

int main() {
  // int s, t;

  // if ((s= establish(PORTNUM, 1)) < 0) {  /* binder is calling to establish a listening socket */
  //   perror("establish");
  //   exit(1);
  // }

  // //printf("Binder started loopin'\n");
  // //signal(SIGCHLD, fireman);           /* this eliminates zombies */

  // for (;;) {                          /* loop for phone calls */
  //   if ((t= get_connection(s)) < 0) { /* get a connection */
  //     if (errno == EINTR)             /* EINTR might happen on accept(), */
  //       continue;                     /* try again */
  //     perror("accept");               /* bad */
  //     exit(1);
  //   }
  //   switch(fork()) {                  /* try to handle connection */
  //   case -1 :                         /* bad news.  scream and die */
  //     perror("fork");
  //     close(s);
  //     close(t);
  //     exit(1);
  //   case 0 :                          /* we're the child, do something */
  //     close(s);
  //     //work(t);
  //     exit(0);
  //   default :                         /* we're the parent so look for */
  //     close(t);                       /* another connection */
  //     continue;
  //   }
  // } 

  int argTypes0[5];
   argTypes0[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16); 
   argTypes0[1] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16) | 23;
   argTypes0[2] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16);
   argTypes0[3] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 23;
   argTypes0[4] = 0;
  // printf("Input mask is: %d\n", INPUT_MASK);
  // printf("Output mask is: %d\n", OUTPUT_MASK);
  // printf("Type mask is: %x\n", mask(16, 24));
  // printf("Array mask is: %x\n", mask(0, 16));


  int argTypes1[5];
  argTypes1[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16); 
  argTypes1[1] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16) | 23;
  argTypes1[2] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_INT << 16);
  argTypes1[3] = (1 << ARG_INPUT)  | (1 << ARG_OUTPUT) | (ARG_LONG << 16) | 23;
  argTypes1[4] = 0;
  

  // bool b = 10;
  // cout << "This bool is true " << b << endl;

  Arg* someArg1 = new Arg(&argTypes0[3]);
  Arg* someArg2 = new Arg(&argTypes0[1]);


  // printf("The arg is inp %d\n", someArg->input);
  // printf("The arg is out %d\n", someArg->output);
  // printf("The type is: %d\n", someArg->type);
  // printf("The size of arr: %d\n", someArg->arrSize);

  char hostname[64] = "saiprasad";
  char derp[64] = "saiprasad";
  
  Func* someFunc = new Func(hostname, argTypes0, N_ELEMENTS(argTypes0));
  cout << "The name of the func is " << someFunc->name << endl;
  cout << "The size of the args is " << someFunc->arguments.size() << endl;
  cout << "The first argument is an array " << someFunc->arguments[0]->arrSize << endl;
  cout << "The second argument is an array " << someFunc->arguments[1]->arrSize << endl;

  cout << "Let's compare the two args " << someArg1->equals(someArg2) << endl;

  Func* otherFunc = new Func(derp, argTypes1, N_ELEMENTS(argTypes1));
  cout << "Let's compare functions now " << someFunc->equals(otherFunc) << endl;

  // printf("argTypes 0 - %d \n argTypes 1 - %d\n", argTypes[0], argTypes[1]);
  // int head[2];
  // head[0] = 0;
  // head[1] = RPC_TERMINATE;
  // int len = sizeof(head);

  // printf("The size of head is %d\n", len); 


  // // GOOD CODE
  int sockfd, sockIncomingFd;
  sockfd = BinderServer::getInstance()->startServer();


  sockIncomingFd = wait_for_conn(sockfd);

  // int a = 4;
  // int b = 5;

  // send(sockIncomingFd, &a, sizeof(a), 0);
  // send(sockIncomingFd, &b, sizeof(b), 0);

  // int* head = read_head(sockToClientfd);
  // int len = head[0];
  // int type = head[1];
  // printf("Received message (head).. len=%d and type=%d\n", len, type);

  BinderServer::getInstance()->read_message(sockIncomingFd);

  close(sockIncomingFd);
  close(sockfd);


  return 0;

}

