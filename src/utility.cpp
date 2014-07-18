#include <vector>
#include <list>
#include <string>
#include <iostream>

#include "message_protocol.h"
#include "utility.h"

using namespace std;

ServerPortCombo::ServerPortCombo(char* n, unsigned int p) {
  name.assign(n);
  port = p;
}

ServerPortCombo::~ServerPortCombo() {
  cout << "~ServerPortCombo :: " << "server/port destructor" << endl;
}

int ServerPortCombo::equals(ServerPortCombo* other) {
  if (name.compare(other->name) != 0) {
    return -1;
  } 
  if (port != other->port) {
    return -1;
  }
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
        return 2; // some kind of warning saying that the client array is greater than the max the server would like to allocate
      } 
    }
  }
  return 0; // if you made it this far, you are okay
}

// static Arg* Arg::parseArg(int* arg) {
//   return new Arg((*arg & INPUT_MASK) >> ARG_INPUT, 
//                  (*arg & OUTPUT_MASK) >> ARG_OUTPUT, 
//                  (*arg & TYPE_MASK) >> 16,
//                  (*arg & ARRAY_MASK));
// }

Func::Func(char* funcName, int argTypes[], int sizeOfArgTypes) {
  name.assign(funcName);
  arguments.reserve(sizeOfArgTypes - 1);

  int* iterator = argTypes;
  while(*iterator != 0) {
    arguments.push_back(new Arg(iterator));
    iterator++; 
  }
}

Func::~Func() {
  cout << "~Func :: " << "Func destructor for funcName = " << name << endl;
  for (vector<Arg*>::iterator it = arguments.begin() ; it != arguments.end(); ++it) {
    delete *it;
  }
  arguments.clear();
}

int Func::equals(Func* other) {
  int warningFlag = 0;
  if (name.compare(other->name) != 0) {
    return -1;
  } 
  if (arguments.size() != other->arguments.size()) {
    return -1;
  } else {
    for (int i = 0; i < arguments.size(); i++) {
      if (arguments[i]->equals(other->arguments[i]) < 0) { // one of the arguments don't match 
        return -1;
      } else if (arguments[i]->equals(other->arguments[i]) > 0) {
        warningFlag = arguments[i]->equals(other->arguments[i]); // generated warning flag
      }
    }
    if (warningFlag != 0) {
      return warningFlag; // all the arguments and func names match, but client is requesting larger array than server wants to allocate
    }
  }
  return 0;
}

