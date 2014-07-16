#ifdef __cplusplus
extern "C" {
#endif

// binder/server
#define RPC_REGISTER          1
#define RPC_REGISTER_SUCCESS  2
#define RPC_REGISTER_FAILURE  3 

// client/binder
#define RPC_LOC_REQUEST       4
#define RPC_LOC_SUCCESS       5
#define RPC_LOC_FAILURE       6

// client/server
#define RPC_EXECUTE           7
#define RPC_EXECUTE_SUCCESS   8
#define RPC_EXECUTE_FAILURE   9

// terminate
#define RPC_TERMINATE         10


#ifdef __cplusplus
}
#endif