#include <vector>
#include <list>
#include <string>
#include <iostream>

#include "message_protocol.h"
#include "utility.h"

using namespace std;

ServerPortCombo::ServerPortCombo(char* n, unsigned short p) {
  name.assign(n);
  port = p;
}

ServerPortCombo::ServerPortCombo(char* n, unsigned short p, int s) {
  cout << "ServerPortCombo(1,2,3)" << endl;
  name.assign(n);
  port = p;
  sockfd = s;
  cout << "ServerPortCombo(1,2,3) " << name << " " << port << " " << sockfd << endl;
}

ServerPortCombo::~ServerPortCombo() {
  cout << "~ServerPortCombo :: " << "destructor for server name: " << name << " and port " << port << endl;
}

int ServerPortCombo::equals(ServerPortCombo* other) {
  cout << "s:p equality :: " << endl;
  if (name.compare(other->name) != 0) {
    return -1;
  } 
  if (port != other->port) {
    return -1;
  }
  cout << "s:p equality :: " << " server/port MATCH! " << endl;
  return 0; // if you made it here, then they're both equal
}


ClientResp::ClientResp(int res, ServerPortCombo* s) {
  respCode = res;
  server = s;
}

ClientResp::~ClientResp() {
  cout << "~ClientResp :: " << "client resp destructor" << endl;
}


Arg::Arg(int* arg) {
  input = (*arg & INPUT_MASK) >> ARG_INPUT;
  output = (*arg & OUTPUT_MASK) >> ARG_OUTPUT;
  type = (*arg & TYPE_MASK) >> 16;
  arrSize = (*arg & ARRAY_MASK);
}

Arg::~Arg() {
  cout << "~Arg :: " << "arg destructor -- bottom of the stack" << endl;
}

int Arg::equals(Arg* other) {
  cout << "Arg::equals :: " << endl;
  if (input != other->input) {
    return -1;
  } else if (output != other->output) {
    return -1;
  } else if (type != other->type) {
    return -1;
  } else { // checked first three members, only array sizes remain
    if (arrSize == 0) { // client method is a scalar
      if (other->arrSize > 0) {
        return -1; // registered method has array
      } 
    } else if(other->arrSize == 0) {
      if (arrSize > 0) {
        return -1; // we are an array but the registered method is a scalar
      } 
    } else {
      if (arrSize > other->arrSize) {
        cout << "Arg::equals :: " << " and we have a MATCH" <<endl;
        return 2; // some kind of warning saying that the client array is greater than the max the server would like to allocate
      } 
    }
  }
  cout << "Arg::equals :: " << " and we have a MATCH" <<endl;
  return 0; // if you made it this far, you are okay
}


Func::Func(char* funcName, int argTypes[], int sizeOfArgTypes) {
  cout << "Func() :: " << funcName << " and size of input args " << sizeOfArgTypes << endl; 
  name.assign(funcName);
  arguments.reserve(sizeOfArgTypes - 1);

  int* iterator = argTypes;
  while(*iterator != 0) {
    arguments.push_back(new Arg(iterator));
    iterator++; 
  }
  cout << "Func() :: " << "Total args for this func is: " << arguments.size() << endl;
}

Func::~Func() {
  cout << "~Func :: " << "Func destructor for funcName = " << name << endl;
  for (vector<Arg*>::iterator it = arguments.begin() ; it != arguments.end(); ++it) {
    delete *it;
  }
  arguments.clear();
}

int Func::equals(Func* other) {
  cout << "Func::equals :: " << endl;
  int warningFlag = 0;
  if (name.compare(other->name) != 0) {
    return -1;
  } 

  if (arguments.size() != other->arguments.size()) {
    return -1;
  } else {
    
    for (int i = 0; i < arguments.size(); i++) {
      cout << "Func::equals :: " << "Checking argument number: " << i << endl;
      
      if (arguments[i]->equals(other->arguments[i]) < 0) { // one of the arguments don't match 
        cout << "Func::equals :: " << "arg:" << i << " did not match" << endl;
        return -1;
      } else if (arguments[i]->equals(other->arguments[i]) >= 0) {
        warningFlag = arguments[i]->equals(other->arguments[i]); // generated warning flag
      }
    }
  }
  cout << "Func::equals :: " << " and we have a MATCH <<<<>>>>>" <<endl;
  return warningFlag;
}

// create a custom mask on the 32 bit integer
int mask(int start, int finish) {
  int res = 0; 
  for (int i = start; i < finish; i++) { 
    res = res | (1 << i); 
  } 
  return res;
}

