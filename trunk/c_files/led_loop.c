#include "io430.h"
#include "in430.h"
int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;
  int i;
  P1DIR |= 0x03;
  while (1) {
    P1OUT ^= 0x03;
    for (i=0;i<10000;i++) {
      __no_operation();
    }
  }
}