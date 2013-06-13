
#define RED_LED 0x01
#define GREEN_LED 0x02

#define SECOND_PART 5

#define LUX_SENSOR 0x00
#define HUM_TEMP_SENSOR 0x01
#define TEMP_SENSOR 0x02
#define ACCELEROMETER 0x04

//#define MASTER_PROBE_SET_PIN    0x00
#define MASTER_PROBE_SET_PIN_MASK     0x01
//#define MASTER_PROBE_TEST_PIN   0x01
#define MASTER_PROBE_TEST_PIN_MASK    0x02

#define MASTER_PROBE_SET_PORT_DIR   P3DIR
#define MASTER_PROBE_TEST_PORT_DIR  P3DIR

#define MASTER_PROBE_SET_PORT   P3OUT
#define MASTER_PROBE_TEST_PORT  P3IN
#define MASTER_PROBE_TEST_PORT_RESET  P3OUT
//char transmissionIsInitialized;

void setResetGreenLed (char set);
void setResetRedLed (char set);
void toggleGreenLed(void);
void toggleRedLed(void);

void initBoard();
void initTimer(void);
void USARTInit ();

void radioOn();
void radioOff();