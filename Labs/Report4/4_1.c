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

// This function writes to the Interrupt Signal Response via the __interrupt return type
#pragma vector = TIMER0_A1_VECTOR
__interrupt void signal_response()
{
    // Interrupt response goes here
    // On any given interrupt toggle led
    P1OUT ^= red;
    // clearing the time since the interrupt occured
    TA0CTL &= ~TAIFG; // use TAIFG by default when timer is in continuous
}

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;     // opening gpio

  // setting direction to inputs and outputs of red, green, and buttons
  P1DIR |= red;

  // Setting green led as outputs
  P1OUT &= ~red;

  // configuring clock to 32khz
  config_ACLK_to_32KHz_crystal();

  // Configuring Timer_A
  TA0CTL = TASSEL_1 | ID_0 | MC_2 | TACLR | TAIE;
  // TAIE enables interrupt for rollback to 0 by default

  // clearing flag at start just in case TACLR fails, lmao
  TA0CTL &= ~TAIFG;

  _enable_interrupts(); // allows for interrupts globally hardware wise

  // P1IFG is port 1's 8-bit register for interrupts for 8 individual interrupt use-cases

  for (;;)
  {
      // Infinite loop
  }
}
