#ifdef __cplusplus
extern "C" {
#endif

// binder/server
#define RPC_REGISTER        1

// client/binder
#define RPC_LOC_REQUEST     2
#define RPC_LOC_SUCCESS     3
#define RPC_LOC_FAILURE     4

// client/server
#define RPC_EXECUTE         5
#define RPC_EXECUTE_SUCCESS 6
#define RPC_EXECUTE_FAILURE 7

// terminate
#define RPC_TERMINATE       8

#define SIZE_HEAD           2

#ifdef __cplusplus
}
#endif