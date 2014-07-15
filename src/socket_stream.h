#ifdef __cplusplus
extern "C" {
#endif

extern int establish(unsigned short portnum, int binder_caller);

extern int get_connection(int socket);

extern int call_socket(char* hostname, unsigned short portnum);

extern int read_data(int socket, char* buf, int n);

extern int write_data(int socket, char* buf, int n);

extern void fireman(void);

#ifdef __cplusplus
}
#endif