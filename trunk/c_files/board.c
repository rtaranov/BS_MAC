#include "board.h" 
#include "mrfi.h"
#include "I2C.h"
#include "Sensors.h"
#include "txrx_simple.h"

char transmissionIsInitialized = 0; //1-Was Transmitted; 0-No Transmission Was
extern unsigned char sensors;

void toggleRedLed(void)
{
  P1OUT ^= RED_LED;
}

void toggleGreenLed(void)
{
  P1OUT ^= GREEN_LED;
}

void setResetRedLed (char set)
{
  if (set)
    P1OUT |= RED_LED;
  else
    P1OUT &= ~RED_LED;
}

void setResetGreenLed (char set)
{
  if (set)
    P1OUT |= GREEN_LED;
  else
    P1OUT &= ~GREEN_LED;
}

void initTimer(void)
{ 
/*  
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  TBCCTL0 = CCIE;                           // TBCCR0 interrupt enabled
  TBCCR0 = 50000;
  //TBCCR0 = 62000;
  TBCTL = TBSSEL_2 + MC_1;                  // SMCLK, upmode
*/
  TBR = 0;  
  TBCCTL0 = CCIE;                           // TBCCR0 interrupt enabled  
  TBCCR0 = 62500;
  TBCTL = TBSSEL_2 + MC_1 + ID_3 + CNTL_0;  // SMCLK, upmode, 8_Divider
}


#define ACCEL 0

extern int t_temp, t_hum;
void diagSensors()
{
  sensors = 0;
  
  //WDTCTL = WDTPW + WDTHOLD;   
     
#if (ACCEL == 1)
  sensors |= (1<<ACCELEROMETER);
#else
  //if(write_byte (LUX_WRITE_ADDRESS, COM_REG))  
  if((write_byte (LUX_WRITE_ADDRESS, COM_REG)) || (write_byte (LUX_WRITE_ADDRESS, POW_UP)))
    sensors &= ~(1<<LUX_SENSOR);    
  else
    sensors |= (1<<LUX_SENSOR);    
  
  if(write_byte_tmp (TEMP_READ_ADDRESS))
    sensors &= ~(1<<TEMP_SENSOR);    
  else
    sensors |= (1<<TEMP_SENSOR);
  
  s_connectionreset();
  if(s_write_byte(MEASURE_TEMP))
  {
    s_connectionreset(); 
    sensors &= ~(1<<HUM_TEMP_SENSOR);
  }
  else
  {
    sensors |= (1<<HUM_TEMP_SENSOR);
  }
#endif  
}

void initBoard()
{
  BSP_Init();
  USARTInit();
  diagSensors();
  
  TXString("Program Started!\r\n", sizeof("Program Started!\r\n"));
  
  P1REN |= 0x04; //Vklju4itj pull-up rezistor
  P1IE |= 0x04; //Vklju4itj prerivnie
}

void USARTInit ()
{
  P3SEL    |= 0x30;     // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1  = UCSSEL_2; // SMCLK
/* 
  UCA0BR0   = 0x45;     // uart0 8000000Hz 115107bps
  UCA0BR1   = 0x00;
  UCA0MCTL  = 0xAA;                     
*/
 
  UCA0BR0   = 0x41;     // 9600 from 8Mhz
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     

  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt  
}

void radioOff()
{
  if (transmissionIsInitialized)
  {
    transmissionIsInitialized = 0;
    Mrfi_RxModeOff();
    MRFI_Sleep();  
  }
}


void radioOn()
{
  if(!transmissionIsInitialized)
  {
    transmissionIsInitialized = 1;
  
    MRFI_Init();
    MRFI_WakeUp();
    MRFI_RxOn();
  }
}