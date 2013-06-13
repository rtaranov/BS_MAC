

#define PACKET_TYPE_IDX 0

#define NODE_INITIALIZATION_PACKET_ARRIVED 0
#define NODE_CAN_READ_TRANSMIT_DATA 1
#define MASTER_PACKET_ARRIVED 2
#define CAN_MAKE_NEW_CONVERSION 3

void processReceivedBuffer(void);

void saveNetInfo();

void determineInitSlotNodeSlot(void);
void returnSlotNumberForNode (void);
char checkIfNodeIsInNet();
unsigned char returnAmountOfNodesInNet();