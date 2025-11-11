#include "msp430fr6989.h"

#define redLED BIT0
#define greenLED BIT7
#define S1 BIT1
#define S2 BIT2

// tracks system state between not flashing (0) and flashing (1)
volatile unsigned int state = 1;

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
  // Configure WDT & GPIO
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;

  // Configure LEDs
  P1DIR |= redLED;
  P9DIR |= greenLED;
  P1OUT &= ~redLED;
  P9OUT &= ~greenLED;

  // Configure buttons
  P1DIR &= ~(S1 | S2);
  P1REN |= (S1 | S2);
  P1OUT |= (S1 | S2);
  P1IFG &= ~(S1 | S2); // Flags are used for latched polling

  config_ACLK_to_32KHz_crystal();

  TA0CCR0  = 819;  // 0.1s
  TA0CCTL0 = CCIE; // Enabling channel 0 interrupt

  TA0CCR1  = 4096; // 0.5s
  TA0CCTL1 = CCIE; // enabling interrupt on channel 1

  TA0CCR2  = 32768; // 1s
  TA0CCTL2 = CCIE; // enabling interrupt on channel 2
  
  //       ACLK       /4     Continuous  Clear TAR
  TA0CTL = TASSEL_1 | ID_2 | MC_2      | TACLR;

  _low_power_mode_3(); // We only need ACLK

  return 0;
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR()
{
  P1OUT ^= redLED;
  TA0CCR0 += 819; // 0.1s
  // clearing the flag
  TA0CCTL0 &= ~CCIFG;
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void T0A1_ISR()
{
  // channel 1
  if (TA0CCTL1 & CCIFG)
  {
    P9OUT ^= greenLED;
    TA0CCR1 += 4096; // 0.5s
    // clearing the flag
    TA0CCTL1 &= ~CCIFG;
  }

  // channel 2
  if (TA0CCTL2 & CCIFG)
  {
    TA0CCR2 += 32768; // 4s

    // clearing the flag
    TA0CCTL2 &= ~ CCIFG;
  
    // checking state
    if (state)
    {
      state = 0;

      TA0CCTL0 &= ~CCIE;
      TA0CCTL1 &= ~CCIE;
      TA0CCR0  += 32768;
      TA0CCR1  += 32768;

      P1OUT &= ~redLED;
      P9OUT &= ~greenLED;
    }
    else
    {
      // state was not flashing and must be turned on to flashing
      state = 1;
      TA0CCTL0 |= CCIE;
      TA0CCTL1 |= CCIE;

      TA0CCTL0 &= ~CCIFG;
      TA0CCTL1 &= ~CCIFG;
    }
  }
}
