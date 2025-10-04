#include <msp430.h>

#define redLED BIT0
#define greenLED BIT7

// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal() 
{
  // By default, ACLK runs on LFMODCLK at 5MHz/128 = 39 KHz
  // Reroute pins to LFXIN/LFXOUT functionality
  PJSEL1 &= ~BIT4;
  PJSEL0 |= BIT4;
  // Wait until the oscillator fault flags remain cleared
  CSCTL0 = CSKEY; // Unlock CS registers
  do 
	{
    CSCTL5 &= ~LFXTOFFG; // Local fault flag
    SFRIFG1 &= ~OFIFG;   // Global fault flag
  } while ((CSCTL5 & LFXTOFFG) != 0);
  CSCTL0_H = 0; // Lock CS registers
  return;
}

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
  PM5CTL0 &= ~LOCKLPM5;     // Disable GPIO power-on default high-impedance mode
  // assign redLED and greenLED outputs
  P1DIR |= redLED;
  P9DIR |= greenLED;
  // turn off LEDs by default
  P1OUT &= ~redLED;
  P9OUT &= ~greenLED;
  // calling function to configure ACLK
  config_ACLK_to_32KHz_crystal();
  // configure Timer_A
  // use ACLK, divide by 1, continuous mode, clear TAR
  TA0CTL = TASSEL_1 | ID_3 | MC_2 | TACLR;
  // Changing ID_0 will make the timer longer
  // ensure flag is cleared before running infinite for loop
  TA0CTL &= ~TAIFG;
  for (;;)
	{
    while ((TA0CTL & TAIFG) != TAIFG)
		{
    }; // delay until flag is set
    P1OUT ^= redLED;  // toggle LED
    TA0CTL &= ~TAIFG; // reset flag
  } // end infinite for loop
} 
