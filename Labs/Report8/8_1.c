#include <msp430fr6989.h>
#include <stdint.h>

/*
 * ADC Moment!!
 * - First we need to choose lower and upper voltage references (Vr- and Vr+)
 * - To find the output result we simply find the 2^n*(Vin - Vr-)/(Vr+ - Vr-)
 *   - BUT, if Vr- is ground, then the result is 2^n*(Vin)/(Vr+)
 *
 *   Sample-and-Hold Time (SHT) for the joystick
 *   - 12bit resolution (2^12 = 4096)
 *   - 
*/

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
    SFRIFG1 &= ~OFIFG; // Global fault flag
  }
  while((CSCTL5 & LFXTOFFG) != 0);

  CSCTL0_H = 0; // Lock CS registers
  return;
}

int main(void)
{
  // Enabling the leds and other stuff
  WDTCTL   = WDTPW | WDTHOLD; // Stop WDT
  PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO pins

  // Actual logic for selection
  for (;;)
  {
  }
}

