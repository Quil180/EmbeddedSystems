#include <inttypes.h>
#include <msp430.h>
#define redLED BIT0
#define greenLED BIT7
#define but1 BIT1
#define but2 BIT2

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

int main(void) {
  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;     // opening gpio

	// setting direction to inputs and outputs of red, green, and buttons
  P1DIR |= redLED;
  P9DIR |= greenLED;
  P1DIR &= ~(but1 | but2);

	// Setting green led as outputs
  P1OUT &= ~redLED;
  P9OUT &= ~greenLED;
  P1OUT |= but1 | but2;

	// Setting resistor
  P1REN |= but1 | but2;

	// Setting the clock
  config_ACLK_to_32KHz_crystal();

  for (;;) {
    while ((P1IN & but1) != 0) 
		{
			// While but1 is pressed, do nothing lmao
    }
    TA0CTL = TASSEL_1 | ID_0 | MC_2 | TACLR; // setting the clock thing to continiuous.
    TA0CTL &= ~TAIFG; // AND with inverse of mask to clear the bit
    while (((P1IN & but1) == 0) && (TA0CTL & TAIFG) == 0) 
		{
			// button1 is not pressed and no overflow occured, loop and do nthing
    }
    if ((TA0CTL & TAIFG) != 0)
		{
			// overflo of timer occured
      P9OUT |= greenLED; // turn on green led.
      while ((P1IN & but2) != 0)
			{
				// while the button2 not pressed, wait
      }
			// button2 was pressed therefore clear green led and reset timer
      P9OUT &= ~greenLED;
      TA0CTL &= ~TAIFG; // AND with inverse of mask to clear the bit
    } 
	  else
		{
			// overflow did not occur therefore flash green led correct time
      TA0CCR0 = TA0R;
      TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR; // timer now using up mode with end time = time
      TA0CTL &= ~TAIFG; // AND with inverse of mask to clear the bit
											
      P1OUT |= redLED;
      while ((TA0CTL & TAIFG) == 0) 
			{
      }
      TA0CTL &= ~TAIFG; // AND with inverse of mask to clear the bit
      P1OUT &= ~redLED;
    }
    TA0CTL &= ~TAIFG; // AND with inverse of mask to clear the bit
    TA0CTL = MC_0 | TACLR;
  }
}
