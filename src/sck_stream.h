#ifdef __cplusplus
extern "C" {
#endif

#define N_ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

// blocking recv of head of message used by all processes to read messages
extern int* read_head(int sockfd);

extern int call_sock(char hostname[], char port[]);

extern int read_data(int socket, char* buf, int n);

extern int write_data(int socket, char* buf, int n);

extern int setup_server(char port[]);

extern int wait_for_conn(int sockfd); 

extern int send_terminate(int sockfd); // sent by client to binder and binder to server

// messages sent by client
extern int send_loc_request(int sockfd, char funcName[], int argTypes[], int sizeOfArgTypes); //to binder

extern int send_execute(int sockfd, char hostname[], unsigned short port, int argTypes[], int sizeOfArgTypes, void** args, int sizeOfArgs); //to server

// messages sent by binder
extern int send_register_success(int sockfd, int warningFlag); // to server

extern int send_register_failure(int sockfd, int reasonCode); // to server

extern int send_loc_success(int sockfd, char hostname[], unsigned short port); // to client

extern int send_loc_failure(int sockfd, int reasonCode); // to client

// messages sent by server
extern int send_register(int sockfd, char server_identifier[], unsigned short port, char funcName[], int argTypes[], int sizeOfArgTypes); //to binder

extern int send_execute_success(int sockfd, char funcName[], int argTypes[],  int sizeOfArgTypes, void** args, int sizeOfArgs); //to client

extern int send_execute_failure(int sockfd, int reasonCode); // to client

// used by binder and server to know themselves (hostname and port)
extern int addrAndPort(int sockfd, char hostname[], unsigned short* port);

#ifdef __cplusplus
}
#endif