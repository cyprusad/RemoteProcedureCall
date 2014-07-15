#ifdef __cplusplus
extern "C" {
#endif

extern int call_sock(char hostname[], char port[]);

extern int read_data(int socket, char* buf, int n);

extern int write_data(int socket, char* buf, int n);

extern void fireman(void);

extern int setup_server(char port[], int binder_caller);

extern int wait_for_conn(int sockfd); 

#ifdef __cplusplus
}
#endif