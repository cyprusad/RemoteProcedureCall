#ifdef __cplusplus
extern "C" {
#endif

extern int call_sock(char hostname[], char port[]);

extern int read_data(int socket, char* buf, int n);

extern int write_data(int socket, char* buf, int n);

extern void fireman(void);

extern int setup_server(char port[]);

extern int wait_for_conn(int sockfd); 

extern int read_message(int sockfd);

extern int send_terminate(int sockfd);

extern int addrAndPort(int sockfd, char hostname[], unsigned short* port);

#ifdef __cplusplus
}
#endif