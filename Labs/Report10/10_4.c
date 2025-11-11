#include "msp430fr6989.h"
#include <stdint.h>

#define redLED BIT0
#define greenLED BIT7
#define S1 BIT1
#define S2 BIT2

// We love UART
#define FLAGS UCA1IFG
#define RXFLAG UCRXIFG
#define TXFLAG UCTXIFG
#define TXBUFFER UCA1TXBUF

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

void initialize_uart(void)
{
  // Configuring the pins to use backchannel uart (SAME)
  P3SEL1 &= ~(transmit | recieve);
  P3SEL0 |= (transmit | recieve);

  // Setting the clock to ACLK (SEL_1 and not SEL_2 [SMCLK])
  UCA1CTLW0 |= UCSSEL_1;

  // Setting the dividers and enabling oversampling
  UCA1BRW = 6; // SAME DIVIDER
  // setting the modulators and such
  UCA1MCTLW = UCBRS1 | UCBRS2 | UCBRS3 | UCBRS5 | UCBRS6 | UCBRS7;

  // Exiting the reset state
  UCA1CTLW0 &= ~UCSWRST;
  return;
}

void uart_write_char(volatile unsigned char ch)
{
  while (!(FLAGS & TXFLAG))
  {
    // Wait for transmission that is ongoing to complete
  }

  TXBUFFER = ch;
  return;
}

void uart_write_string(char *string)
{
  int i; // counter
  for (i = 0; i < strlen(string); i++)
  {
    uart_write_char(string[i]);
  }
  return;
}

void uart_write_uint16(uint16_t number)
{
  // Converting the number via snprintf
  char buffer[6]; // 5 characters is the max amount of characters for 65,536
  custom_itoa(number, buffer);
  uart_write_string(buffer);
  return;
}

int main(void)
{
  // Configure WDT & GPIO
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;

  // Configure buttons
  P1DIR  &= ~S1; // input
  P1SEL1 &= ~S1; // 0
  P1SEL0 |= S1;  // 1
  P1REN  |= S1; // enable pull-up
  P1OUT  |= S1; // selecting pull-up

  initialize_uart();
  config_ACLK_to_32KHz_crystal();

  /*
   * CM_3:   Capture on both edges
   * CCIS_0: Capture input A (P1.1)
   * CAP:    Enable Capture Mode
   * CCIE:   Enable Interrupts
   * SCS:    Synchronous Capture (sync w/ clock)
  */
  TA0CCTL2 = CM_3 | CCIS_0 | CAP | CCIE | SCS;

  //       ACLK       /1     Continuous  Clear TAR
  TA0CTL = TASSEL_1 | ID_0 | MC_2      | TACLR;

  __bis_SR_register(LPM3_bits | GIE);

  return 0;
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void T0A1_ISR()
{
  if (TA0CCTL2 & CCIFG)
  {
    uint16_t time = TA0CCR2;

    uart_print_string("Time: ");
    uart_print_uint16(time);
    uart_printf_string("\r\n");

    // delay of 20ms
    __delay_cycles(20000);

    TA0CCTL2 &= ~CCIFG;
  }
}
