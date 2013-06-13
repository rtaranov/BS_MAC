#include "I2C.h"
#include "mrfi.h"
#include "txrx_simple.h"
#include "math.h"

unsigned int RxByteCtr = 2;
unsigned int RxWord;
unsigned char TXData;

unsigned char TXByteCtr = 2;

unsigned char error_ = 0;
unsigned char ii;


void START () //SDA HIGH to LOW, while SCL HIGH
{
  P2DIR |= 0x03;
  P2OUT |= SDA;
  P2OUT |= SCL;
  __delay_cycles (10);
  P2OUT &= ~SDA;
   __delay_cycles (10);
  P2OUT &= ~SCL;
}
void STOP () //SDA LOW to HIGH, while SCL is HIGH
{
  P2DIR |= 0x03;
  P2OUT &= ~SDA;
  P2OUT |= SCL;
  __delay_cycles (10);
  P2OUT |= SDA;
}

unsigned char write_byte (unsigned char addr, unsigned char val)
{
  //unsigned char i, error = 0;  
  //unsigned char i;
  error_ = 0;  
  
  P2DIR |= 0x03;
  START ();
//-----------address transfer-------------------------------------------------
  for (ii = 0x80; ii > 0; ii /= 2)             //shift bit for masking
  { 
    if (ii & addr) P2OUT |= SDA;    //masking value with i , write to SENSI-BUS
    else P2OUT &= ~SDA;                        
    __delay_cycles (8);                       
    P2OUT |= SCL;                           
    __delay_cycles (8);       	
    P2OUT &= ~SCL;
    __delay_cycles (8);             
  }
//--------clk for ACK----------------------------------------------------------  
  P2DIR &= ~SDA;                    //release DATA-line/ SDA direction to input
  __delay_cycles (8);
  
  P2OUT |= SCL; 
  __delay_cycles (8);               //clk #9 for ack 
  if (SDA & P2IN)                   //check ack
    error_ = 1;                       
  else error_ = 0;
  __delay_cycles (8);
  P2OUT &= ~SCL; 
//--------command transfer-----------------------------------------------------
  P2DIR |= SDA;
  for (ii = 0x80; ii > 0; ii /= 2)             
  { 
    if (ii & val) P2OUT |= SDA;
    else P2OUT &= ~SDA;                        
    __delay_cycles (8);                       
    P2OUT |= SCL;                           
    __delay_cycles (8);       	
    P2OUT &= ~SCL;
    __delay_cycles (8);             
  }
//--------clk for ACK----------------------------------------------------------
   P2DIR &= ~SDA;                    //release DATA-line/ SDA direction to input
  __delay_cycles (8);
  
  P2OUT |= SCL; 
  __delay_cycles (8);               //clk #9 for ack 
  if (SDA & P2IN)                   //check ack
    error_ = 1;                       
  else error_ = 0;
  __delay_cycles (8);
  P2OUT &= ~SCL;
  
  STOP ();
  return error_;
}

unsigned char write_byte_tmp (unsigned char value)
{
  START ();
  P2DIR |= 0x03;

  //unsigned char i, error = 0;
  //unsigned char i;
  error_ = 0;
  
  for (ii = 0x80; ii > 0; ii /= 2)             //shift bit for masking
  { if (ii & value) P2OUT |= SDA;    //masking value with i , write to SENSI-BUS
    else P2OUT &= ~SDA;                        
    __delay_cycles (8);                       
    P2OUT |= SCL;                           
    __delay_cycles (8);       	
    P2OUT &= ~SCL;
    __delay_cycles (8);             //observe hold time
  }
  
  P2DIR &= ~SDA;                    //release DATA-line/ SDA direction to input
  __delay_cycles (8);
  
  P2OUT |= SCL; 
  __delay_cycles (8);               //clk #9 for ack 
  if (SDA & P2IN)                   //check ack
    error_ = 1;                       
  else error_ = 0;
  __delay_cycles (8);
  P2OUT &= ~SCL;  
  return error_;
}

unsigned int read_byte (unsigned char ack)
{
  //unsigned char i, val = 0;
  unsigned char val = 0;
  
  P2DIR |= SCL;
  P2DIR &= ~SDA;          
  
  for (ii = 0x80; ii > 0; ii /= 2)           
  { 
    P2OUT |= SCL;                         
    __delay_cycles (8);
    if (SDA & P2IN) val = (val | ii);       
    P2OUT &= ~SCL;  
    __delay_cycles (8);
  }
  
  P2DIR |= SDA;
  if (ack) P2OUT &= ~SDA;            // DATA=!ack; in case of "ack==1" pull down DATA-Line
  else P2OUT |= SDA;
   
  __delay_cycles (8);                
  P2OUT |= SCL;                      
  __delay_cycles (8);           
  P2OUT &= ~SCL; 
  __delay_cycles (8);                         					    
  P2DIR &= ~SDA;
  return val;
}

unsigned int i, val, MSB, LSB;

unsigned int read_word (unsigned char addr)
{
  //unsigned char error=0;
  error_=0;
  
  MSB = 0;
  LSB = 0;
  P2DIR |= 0x03; 
  START ();
//----address transfer---------------------------------------------------------
  for (i = 0x80; i > 0; i /= 2)             //shift bit for masking
  { 
    if (i & addr) P2OUT |= SDA;    //masking value with i , write to SENSI-BUS
    else P2OUT &= ~SDA;                        
    __delay_cycles (8);                       
    P2OUT |= SCL;                           
    __delay_cycles (8);       	
    P2OUT &= ~SCL;
    __delay_cycles (8); 
  }
//--------clk for ACK from slave-----------------------------------------------
  P2DIR &= ~SDA;                    //release DATA-line/ SDA direction to input
  __delay_cycles (8);
  P2OUT |= SCL; 
  __delay_cycles (8);               //clk #9 for ack 
  if (SDA & P2IN)                   //check ack
    error_++;                       
  __delay_cycles (8);
  P2OUT &= ~SCL;
  
//--------MSB byte reading-----------------------------------------------------
  val = 0;
  for (i=0x80;i>0;i/=2)             //shift bit for masking
  { 
    P2OUT |= SCL;                          //clk for SENSI-BUS
    __delay_cycles (8);
    if (SDA & P2IN) MSB=(MSB | i);        //read bit  
    P2OUT &= ~SCL;  
    __delay_cycles (8);
  }
//-----ACK from master---------------------------------------------------------  
  P2DIR |= SDA;
  P2OUT &= ~SDA;
  __delay_cycles (8);                
  P2OUT |= SCL;                      
  __delay_cycles (8);           
  P2OUT &= ~SCL; 
  __delay_cycles (8); 
//--------LSB byte reading-----------------------------------------------------
  P2DIR &= ~SDA;
  val = 0;
  for (i=0x80;i>0;i/=2)             //shift bit for masking
  { 
    P2OUT |= SCL;                          //clk for SENSI-BUS
    __delay_cycles (8);
    if (SDA & P2IN) LSB=(LSB | i);        //read bit  
    P2OUT &= ~SCL;  
    __delay_cycles (8);
  }  
//-----NACK from master---------------------------------------------------------  
  P2DIR |= SDA;
  P2OUT |= SDA;
  __delay_cycles (8);                
  P2OUT |= SCL;                      
  __delay_cycles (8);           
  P2OUT &= ~SCL; 
  __delay_cycles (8); 
  
  STOP ();
  val = 256 * LSB + MSB; //for TLS2560!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //val = 256 * MSB + LSB in other case
  return val;
}

//----------------------------------------------------------------------------------
char s_write_byte(unsigned char value)
//----------------------------------------------------------------------------------
// writes a byte on the Sensibus and checks the acknowledge 
{ 
  P2DIR |= 0x03;
  
  //unsigned char i,error=0; 
  error_=0;
  
  for (ii=0x80;ii>0;ii/=2)             //shift bit for masking
  { if (ii & value) P2OUT |= SDA;           //masking value with i , write to SENSI-BUS
    else P2OUT &= ~SDA;                        
    __delay_cycles (8);                       
    P2OUT |= SCL;                           
    __delay_cycles (8);       	
    P2OUT &= ~SCL;
    __delay_cycles (8);                         //observe hold time
  }
  
  P2DIR &= ~SDA;                      //release DATA-line/ SDA direction to input
  __delay_cycles (8);
  
  P2OUT |= SCL; 
  __delay_cycles (8);                //clk #9 for ack 
  if (SDA & P2IN)           //check ack
    error_ = 1;                       
  else error_ = 0;
  __delay_cycles (8);
  P2OUT &= ~SCL;  
  return error_;
}

//----------------------------------------------------------------------------------
int s_read_byte(unsigned char ack)
//----------------------------------------------------------------------------------
// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1" 
{ 
  //unsigned char i,val=0;
  unsigned char val=0;
  
  P2DIR |= 0x03;
  P2DIR &= ~SDA;                            //release DATA-line
  for (ii=0x80;ii>0;ii/=2)             //shift bit for masking
  { 
    P2OUT |= SCL;                          //clk for SENSI-BUS
    __delay_cycles (8);
    if (SDA & P2IN) val=(val | ii);        //read bit  
    P2OUT &= ~SCL;  
    __delay_cycles (8);
  }
  P2DIR |= 0x03;
  
   if (ack) P2OUT &= ~SDA;                                    // DATA=!ack; in case of "ack==1" pull down DATA-Line
    else P2OUT |= SDA;
   
  __delay_cycles (8);                          //observe setup time
  P2OUT |= SCL;                           //clk #9 for ack
  __delay_cycles (8);          //pulswith approx. 5 us 
  P2OUT &= ~SCL; 
  __delay_cycles (8);                          //observe hold time						    
  P2DIR &= ~SDA;                           //release DATA-line
  return val;
}

//----------------------------------------------------------------------------------

void s_transstart(void)
//----------------------------------------------------------------------------------
// generates a transmission start 
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
{  
  P2DIR |= 0x03; 
  P2OUT |= SDA;
   P2OUT &= ~SCL;                  //Initial state
   
   __delay_cycles (8);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
   P2OUT |= SCL; 
   __delay_cycles (8);
   P2OUT &= ~SDA;
   __delay_cycles (8);
   P2OUT &= ~SCL;  
   __delay_cycles (16);
   P2OUT |= SCL;
   __delay_cycles (8);
   P2OUT |= SDA;	   
   __delay_cycles (8);
   P2OUT &= ~SCL;	   
}

//----------------------------------------------------------------------------------

void s_connectionreset(void)
//----------------------------------------------------------------------------------
// communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart
//       _____________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
{  
  //unsigned char i; 
  
  P2DIR |= 0x03; 
  P2OUT |= SDA;	 
  P2OUT &= ~SCL;                    //Initial state
  for(ii=0;ii<9;ii++)                  //9 SCK cycles
  { 
    P2OUT |= SCL;
    __delay_cycles (8);
    P2OUT &= ~SCL; 
    __delay_cycles (8);
  }
  s_transstart();                   //transmission start
}

//----------------------------------------------------------------------------------
char s_softreset(void)
//----------------------------------------------------------------------------------
// resets the sensor by a softreset 
{ 
  //unsigned char error=0; 
  error_=0;
  
  s_connectionreset();              //reset communication
  error_+=s_write_byte(RESET);       //send RESET-command to sensor
  return error_;                     //error=1 in case of no response form the sensor
}

//----------------------------------------------------------------------------------
char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum)
//----------------------------------------------------------------------------------
// reads the status register with checksum (8-bit)
{ 
  //unsigned char error=0;
  error_=0;
  
  s_transstart();                   //transmission start
  error_=s_write_byte(STATUS_REG_R); //send command to sensor
  *p_value=s_read_byte(ACK);        //read status register (8-bit)
  *p_checksum=s_read_byte(NACK);   //read checksum (8-bit)  
  return error_;                     //error=1 in case of no response form the sensor
}

//----------------------------------------------------------------------------------
char s_write_statusreg(unsigned char *p_value)
//----------------------------------------------------------------------------------
// writes the status register with checksum (8-bit)
{ 
  //unsigned char error=0;
  error_=0;
  
  s_transstart();                   //transmission start
  error_+=s_write_byte(STATUS_REG_W);//send command to sensor
  error_+=s_write_byte(*p_value);    //send value of status register
  return error_;                     //error>=1 in case of no response form the sensor
}


  //int MSB, LSB;

//----------------------------------------------------------------------------------
char s_measure(int *p_value, unsigned char *p_checksum, unsigned char mode)
//----------------------------------------------------------------------------------
// makes a measurement (humidity/temperature) with checksum
{ 
  //unsigned char error=0;
//  int MSB, LSB, i;
  error_=0;
  
  s_transstart();                   //transmission start
  switch(mode){                     //send command to sensor
    case TEMP	: error_+=s_write_byte(MEASURE_TEMP); break;
    case HUMI	: error_+=s_write_byte(MEASURE_HUMI); break;
    default     : break;	 
  }
  P2DIR &= ~SDA; //change direction;
  
  while (1)
  {
    if(! (SDA & P2IN)) break; 
  }
  
  if(P2IN & SDA) error_+=1;                // or timeout (~2 sec.) is reached
  
  MSB = s_read_byte(ACK);    //read the first byte (MSB)
  LSB = s_read_byte(ACK);    //read the second byte (LSB)
  *p_checksum =s_read_byte(NACK);  //read checksum
  *p_value = 256 * MSB + LSB;
  return error_;
}
 		

//const float C1, C2, C3, T1, T2;
/*
const float C1=-2.0468;           // for 12 Bit RH
const float C2=+0.0367;           // for 12 Bit RH
const float C3=-0.0000015955;     // for 12 Bit RH
const float T1=+0.01;             // for 12 Bit RH
const float T2=+0.00008;          // for 12 Bit RH	
*/
float rh;             // rh:      Humidity [Ticks] 12 Bit 
float t;           // t:       Temperature [Ticks] 14 Bit
float rh_lin;                     // rh_lin:  Humidity linear
float rh_true;                    // rh_true: Temperature compensated humidity
float t_C;                        // t_C   :  Temperature [°C]  
  
//----------------------------------------------------------------------------------------
void calc_sth11(float *p_humidity ,float *p_temperature)
//----------------------------------------------------------------------------------------
// calculates temperature [°C] and humidity [%RH] 
// input :  humi [Ticks] (12 bit) 
//          temp [Ticks] (14 bit)
// output:  humi [%RH]
//          temp [°C]
{ 
/*
  const float C1=-2.0468;           // for 12 Bit RH
  const float C2=+0.0367;           // for 12 Bit RH
  const float C3=-0.0000015955;     // for 12 Bit RH
  const float T1=+0.01;             // for 12 Bit RH
  const float T2=+0.00008;          // for 12 Bit RH	

  
  C1=-2.0468;           // for 12 Bit RH
  C2=+0.0367;           // for 12 Bit RH
  C3=-0.0000015955;     // for 12 Bit RH
  T1=+0.01;             // for 12 Bit RH
  T2=+0.00008;          // for 12 Bit RH	

  float rh=*p_humidity;             // rh:      Humidity [Ticks] 12 Bit 
  float t=*p_temperature;           // t:       Temperature [Ticks] 14 Bit
  float rh_lin;                     // rh_lin:  Humidity linear
  float rh_true;                    // rh_true: Temperature compensated humidity
  float t_C;                        // t_C   :  Temperature [°C]
*/
  
  rh=*p_humidity;
  t=*p_temperature;
  
  t_C=t*0.01 - 40.1;                //calc. temperature [°C] from 14 bit temp. ticks @ 5V
  rh_lin=(-0.0000015955)*rh*rh + 0.0367*rh + (-2.0468);     //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(0.01+0.00008*rh)+rh_lin;   //calc. temperature compensated humidity [%RH]
  if(rh_true>100)rh_true=100;       //cut if the value is outside of
  if(rh_true<0.1)rh_true=0.1;       //the physical possible range

  *p_temperature=t_C;               //return temperature [°C]
  *p_humidity=rh_true;              //return humidity[%RH]
}

//extern float dew_point ;
//float k ;
//--------------------------------------------------------------------
float calc_dewpoint(float h,float t)
//--------------------------------------------------------------------
// calculates dew point
// input:   humidity [%RH], temperature [°C]
// output:  dew point [°C]
{ 
  //float k,dew_point ;
  
  //k = (log10(h)-2)/0.4343 + (17.62*t)/(243.12+t);
  //dew_point = 243.12*k/(17.62-k);
  rh = (log10(h)-2)/0.4343 + (17.62*t)/(243.12+t);
  t_C = 243.12*rh/(17.62-rh);//dew_point
  return t_C;
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

