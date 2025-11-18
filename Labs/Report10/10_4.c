#include "msp430fr6989.h"
#include <stdint.h>
#include <string.h>

// UART Channels are P3.4 and P3.5 for transmit and recieve respectively
#define transmit BIT4
#define recieve BIT5

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
  UCA1BRW = 3; // SAME DIVIDER
  // setting the modulators and such
  UCA1MCTLW = 0x9200;

  // Exiting the reset state
  UCA1CTLW0 &= ~UCSWRST;
  return;
}

// Converts an unsigned 16-bit integer to a null-terminated string (base 10).
void custom_itoa(uint16_t number, char *buffer)
{
  unsigned int i = 0;

  // Handle the special case of 0
  if (number == 0)
  {
    buffer[i++] = '0';
    buffer[i] = '\0';
    return;
  }

  // Process individual digits
  while (number > 0)
  {
    int remainder = number % 10;
    buffer[i++] = remainder + '0'; // Convert digit to its ASCII character
    number = number / 10;
  }

  buffer[i] = '\0'; // Null-terminate the string

  // The digits are in reverse order, so we need to reverse the string
  custom_strrev(buffer);
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
  unsigned int i; // counter
  for (i = 0; string[i] != '\0'; i++) 
  {
  uart_write_char(string[i]);
  }
  return;
}

char *uart_write_uint16(unsigned int n)
{
  static char digits[6]; // 6 digits for null terminator
  int i = 5;
  digits[i] = '\0'; // adds null terminator

  if (n == 0) 
  {
    digits[--i] = '0'; // For printing 0's
  } 
  else
  {
    while (n > 0 && i > 0)
    {
      digits[--i] = (n % 10) + '0';
      n /= 10;
    }
  }
  return &digits[i]; // Return pointer to start of the number
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

  config_ACLK_to_32KHz_crystal();
  initialize_uart();


  /*
   * CM_3:   Capture on both edges
   * CCIS_0: Capture input A (P1.1)
   * CAP:    Enable Capture Mode
   * CCIE:   Enable Interrupts
   * SCS:    Synchronous Capture (sync w/ clock)
  */
  TA0CCTL2 = CM_3 | CCIS_0 | CAP | CCIE;

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

    uart_write_string("Time: ");
    uart_write_string(uart_write_uint16(time));
    uart_write_string("\r\n");

    // delay
    __delay_cycles(100);

    TA0CCTL2 &= ~CCIFG;
  }
}
