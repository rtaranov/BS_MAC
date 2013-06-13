
//Define Protocol Features
#define SELF_CLUSTERING 1
#define DEBUG 1
#define RAPID_DEVELOPMENT 0

//-----------------------------------


//Define Node States
#define NULL_STATE 0
#define MASTER_STATE 1
#define NODE_STATE 2

#define NODE_IS_NONE 0
#define NODE_IS_MASTER 1
#define NODE_IS_NODE 2

//-----------------------------------

//Define Algo States
#define INIT_INTO_NET 10
#define MASTER_ELLECTION 11
#define M_SEND_NET_INFO 12
#define M_SEND_DATA 13
#define N_INIT_TO_NET 14
#define N_IF_I_IN_NET 15
#define N_CAN_SEND 16
#define MASTER_DETECTION 17
#define M_WAIT 18
#define INITIALIZATION_CHECK 19

//-----------------------------------


//Define MAC packet Types
#define MASTER_SLOT_PACKET_TYPE 1
#define MASTER_DATA_PACKET_TYPE 2
#define NODE_DATA_PACKET_TYPE 3
#define MASTER_ELECTION_PACKET_TYPE 4
#define NODE_INITIALIZATION_PACKET_TYPE 5
#define DATA_PACKET_TYPE 6

//-----------------------------------


enum {TEMP,HUMI};

#define SET_ 1
#define RESET_ 0