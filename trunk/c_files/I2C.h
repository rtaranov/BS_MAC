#define UART_PORT_SEL P3SEL             // Select the USCI port. Find the IO port 
                                          // in which RX/TX pins are located
                                          // and replace P3 by that port.

#define UART_PIN_TX BIT4                // Transmit pin (P3.4 here)

#define UART_PIN_RX BIT5                // Receive pin (P3.5 here)

extern unsigned char TXData;
unsigned char write_byte (unsigned char addr, unsigned char val);
unsigned int read_word (unsigned char addr);

unsigned char write_byte_tmp (unsigned char value);
unsigned int read_byte (unsigned char ack);

void START ();
void STOP ();

//SHT11 Humi and Temp sensor
void s_connectionreset(void);
char s_softreset(void);
char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum);
char s_write_statusreg(unsigned char *p_value);
char s_measure(int *p_value, unsigned char *p_checksum, unsigned char mode);
void calc_sth11(float *p_humidity ,float *p_temperature);
float calc_dewpoint(float h,float t);
char s_write_byte(unsigned char value);


#define SCL   0x01    // [00000001]
#define SDA   0x02    // [00000010]

#define NACK  0
#define ACK   1
                            //adr 0x29 r/w
//#define RA          0x53    //0101001   1 address for reading
//#define WA          0x52    //0101001   0  address for writing

#define COM_REG     0xA0 
#define POW_UP      0x03

//word protokol
#define READ_CH0    0xAC
#define READ_CH1    0xAE

//byte protocol
#define READ_LOW_CH0    0x8C
#define READ_HIGH_CH0   0x8D
#define READ_LOW_CH1    0x8E
#define READ_HIGH_CH1   0x8F


                            //adr  command  r/w
#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1
#define RESET        0x1e   //000   1111    0




                                  