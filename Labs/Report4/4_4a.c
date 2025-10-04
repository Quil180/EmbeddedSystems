#include <inttypes.h>
#include <msp430fr6989.h>

#define red BIT0

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

// Setting up interrupts via __interrupt
#pragma vector = TIMER0_A1_VECTOR
__interrupt void signal_response()
{
	P1OUT ^= red;
	// clearing the time since the interrupt occured
	TA0CTL &= ~TAIFG; // use TAIFG by default when timer is in continuous
}

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;     // opening gpio

  // setting direction to ouput for red
  P1DIR |= red;

  // Setting red led as off
  P1OUT &= ~red;

  // configuring clock to 32khz
  config_ACLK_to_32KHz_crystal();

  // Configuring Timer_A with interrupt mode enabled
  TA0CTL = TASSEL_1 | ID_0 | MC_2 | TACLR | TAIE;

  // clearing flag as a sanity check
  TA0CTL &= ~TAIFG;

	// Lowest power mode that turns on the ACLK
	_low_power_mode_3();

  for (;;)
  {
      // Infinite loop
  }
}
