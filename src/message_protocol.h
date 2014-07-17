#ifdef __cplusplus
extern "C" {
#endif

#include "rpc.h"

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

// bit masks
#define INPUT_MASK  0x80000000
#define OUTPUT_MASK 0x40000000
#define TYPE_MASK   0xff0000
#define ARRAY_MASK  0xffff

#ifdef __cplusplus
}
#endif