#include "Sensors.h"
#include "I2C.h"
#include "math.h"
#include "mrfi.h"
#include "txrx_simple.h"

extern float lux;
extern float temperature;
/*
static int d0l;
static int d0h;
static int d1l;
static int d1h;
*/
unsigned char msb, lsb;
static float ch0;
static float ch1;
static float value;
unsigned char error;

void takeLuxes()
{
  
  //WDTCTL = WDTPW + WDTHOLD;
  error = 0;
  error += write_byte (LUX_WRITE_ADDRESS, COM_REG);           
  error += write_byte (LUX_WRITE_ADDRESS, POW_UP);           //transfer address
//500 ms delay for measure
  //__delay_cycles (8000000);
  BSP_Delay(500);
  error += write_byte (LUX_WRITE_ADDRESS, READ_CH0);           //reading from CH0
  ch0 = read_word (LUX_READ_ADDRESS);
  
  error += write_byte (LUX_WRITE_ADDRESS, READ_CH1);           //reading from CH1
  ch1 = read_word (LUX_READ_ADDRESS);
  
  
  //lux calculation
  
  value = (float) ch1 / (float) ch0;
  
  if ((value > 0) && (value <= 0.5))                      //CH1=1 CH0=4 (value=0.25) (lux=0.08599)
  lux = (0.0304 * ch0) - (0.062 * ch0 * pow(value, 1.4));
  
  if ((value > 0.5) && (value <= 0.61))                   //CH1=5.5 CH0=10 (value=0.55) (lux=0.0535)
  lux = (0.0224 * ch0) - (0.031 * ch1);
  
  if ((value > 0.61) && (value <= 0.8))                   //CH1=7 CH0=10 (value=0.7) (lux=0.02089)
  lux = (0.0128 * ch0) - (0.0153 * ch1);
  
  if ((value > 0.8) && (value <= 1.3))                    //CH1=12 CH0=10 (value=1.2) (lux=0.00116)
  lux = (0.00146 * ch0) - (0.00112 * ch1);
  
  if (value > 1.3)
  lux = 0;
}


static int tempValue;

void takeTemperature()
{
  //WDTCTL = WDTPW + WDTHOLD;
  //START ();
  error+=write_byte_tmp (TEMP_READ_ADDRESS); 
  msb = read_byte (ACK);
  lsb = read_byte (NACK);
  STOP ();
  tempValue = 256 * msb + lsb;
  tempValue = tempValue >> 4;
  temperature = (float) tempValue * 0.0625;
}

extern int t_temp, t_hum;
extern float dew_point, temperatureSHT11, hum;

void takeHumiAnfTemperature()
{ 
  unsigned char error,checksum;
  //unsigned int i;

  //WDTCTL = WDTPW + WDTHOLD;
  s_connectionreset();

  error=0;
  error+=s_measure(&t_temp,&checksum,TEMP);
  error+=s_measure(&t_hum,&checksum,HUMI);   
  if(error!=0) s_connectionreset();                
  else
  {
    temperatureSHT11 = (float) t_temp;
    hum = (float) t_hum;
    calc_sth11(&hum, &temperatureSHT11);            //calculate humidity, temperature
    dew_point=calc_dewpoint(hum,temperatureSHT11); //calculate dew point
  }
    //1 second delay
  //__delay_cycles (16000000);
}


extern int acc;

void takeAccelerometer()
{ 
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  ADC10CTL0 = ADC10IE + REFON + ADC10ON;
  ADC10AE0 |= 0x01;                         // P2.0 v re#ime ADC
  //P2DIR |= 0x02;                            // Aktiviruem no#ku 2.1 esli +1g
  //P2DIR |= 0x03;                            // Aktiviruem no#ku 2.2 esli +0g
  //P2DIR |= 0x04;                            // Aktiviruem no#ku 2.3 esli -1g

  //for (;;)
  //{
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit

    acc = ADC10MEM;
    
    
/*    
   if ((ADC10MEM > 748)&&(ADC10MEM < 788))  //esli 2.25 +-20 delenij, za#igaem no#ku 2.3
      P2OUT |= 0x04;                       
    else
      P2OUT &= ~0x04;    
    
     if ((ADC10MEM > 833)&&(ADC10MEM < 873))   //esli 2.5 +-20 delenij, za#igaem no#ku 2.2
      P2OUT |= 0x03;                       
    else
      P2OUT &= ~0x03;  
    
    if ((ADC10MEM > 918)&&(ADC10MEM < 958))   //esli 2.75 +-20 delenij, za#igaem no#ku 2.1
      P2OUT |= 0x02;                       
    else
      P2OUT &= ~0x02;  
*/    
  //}
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}