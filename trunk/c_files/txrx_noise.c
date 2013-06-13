#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
volatile int i = 0;
volatile int c = 0;
char buf[100];
  
void print_rssi(int8_t rssi)
{
  char output[] = {" 000 "};
  if (rssi<0) {output[0]='-';rssi=-rssi;}
  output[1] = '0'+((rssi/100)%10);
  output[2] = '0'+((rssi/10)%10);
  output[3] = '0'+ (rssi%10);
  TXString(output, (sizeof output)-1);
}

#define BSP_TIMER_CLK_MHZ   (BSP_CONFIG_CLOCK_MHZ_SELECT)
#define BSP_DELAY_MAX_USEC  (0xFFFF/BSP_TIMER_CLK_MHZ)

int main(void)
{
  uint32_t usec;
  
  BSP_Init();
  MRFI_Init();
  
//  TBCTL |= TBSSEL_2;
  
  //BCSCTL2 |= DIVS_3;
  usec = 1000;
 
  BSP_ASSERT(usec < BSP_DELAY_MAX_USEC);
  TBR = 0;
  
  TBCCTL0 = CCIE;                           // TBCCR0 interrupt enabled
  
  TBCCR0 = 62500;
    //TBCCR0 = usec*BSP_TIMER_CLK_MHZ;
  //TBCCR0 = 62000;
  TBCTL = TBSSEL_2 + MC_1 + ID_3 + CNTL_0;                  // SMCLK, upmode 
  //TBCTL |= MC_1 + TBSSEL_2;                  // SMCLK, upmode 

  P3SEL    |= 0x30;
  UCA0CTL1  = UCSSEL_2;
  UCA0BR0   = 0x41;
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST;
  MRFI_WakeUp();
  __bis_SR_register(GIE);
  TXString("HELLO!\n",sizeof("HELLO!\n"));
  while(1) {
    ;
/*    
    for (channel=0;channel<200;channel++) {
      MRFI_RxIdle();
      mrfiSpiWriteReg(CHANNR,channel);
      MRFI_RxOn();
      rssi=MRFI_Rssi();
      print_rssi(rssi);
    }
    TXString("\n",1);
while(1);
    */
  }
}
void MRFI_RxCompleteISR()
{
}

#pragma vector=TIMERB0_VECTOR
__interrupt void Timer_B (void)   
{
  i++;
  if(i == 16)
  {
    i = 0;
    c++;
    sprintf(buf, "%i\r\n", c);
    TXString(buf,strlen(buf));
  }
}