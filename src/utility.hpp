#include <string>
#include <vector>

#define NTHREADS 100
using namespace std;
// always unique
class ServerPortCombo {
  public:
    string name;
    unsigned short port;
    int sockfd;

    ServerPortCombo(char* n, unsigned short p); 
    ServerPortCombo(char* n, unsigned short p, int s); 

    ~ServerPortCombo(); 

    int equals(ServerPortCombo* other); 
};

class ClientResp {
  public:
    int respCode;
    ServerPortCombo* server;

    // -1 response would mean that no such function is registered with the binder; anything else will give the server/port
    ClientResp(int res, ServerPortCombo* s);

    ~ClientResp(); 
};

class Arg {
  public:
    bool input;
    bool output;
    int type;
    int arrSize;

    Arg(int* arg);

    ~Arg();

    // just checks the equality of argument signatures
    int equals(Arg* other); 

}; 

class Func {
  public:
    string name;
    vector<Arg*> arguments;

    Func(char* funcName, int argTypes[], int sizeOfArgTypes); 

    ~Func(); 

    int equals(Func* other); 
};
