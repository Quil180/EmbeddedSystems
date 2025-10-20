#include <msp430fr6989.h>

#define redLED BIT0 // Red at P1.0
#define greenLED BIT7 // Green at P9.7

// UART Channels are P3.4 and P3.5 for transmit and recieve respectively
#define transmit BIT4
#define recieve BIT5

// WE LOVE DEFINES
#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer

void initialize_uart(void)
{
  // Configuring the pins to use backchannel uart
  P3SEL1 &= ~(transmit | recieve);
  P3SEL0 |= (transmit | recieve);

  // Setting the clock to SMCLK
  UCA1CTLW0 |= UCSSEL_2;

  // Setting the dividers and enabling oversampling
  UCA1BRW = 6;
  // setting the modulators and such
  UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;

  // Exiting the reset state
  UCA1CTLW0 &= ~UCSWRST;
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

volatile unsigned char uart_read_char(void)
{
  if (!(FLAGS & RXFLAG))
  {
    return 0; // no byte was recieved
  }

  // Return the buffer
  volatile unsigned char return_char = RXBUFFER;
  return return_char;
}

int main(void)
{
  // Enabling the leds and other stuff
  WDTCTL = WDTPW | WDTHOLD; // Stop WDT
  PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins
  // Pins as output
  P1DIR |= redLED; 
  P9DIR |= greenLED;
  // Setting both leds to off
  P1OUT &= ~redLED;
  P9OUT &= ~greenLED;

  // doing what the function says
  initialize_uart(); 

  unsigned char count = '0';

  for (;;)
  {
    volatile unsigned char read_char = uart_read_char();
    if (read_char == '1')
    {
      P9OUT |= greenLED;
    }
    else if (read_char == '2')
    {
      P9OUT &= ~greenLED;
    }

    if (count > '9')
    {
      count = '0';
    }
    uart_write_char(count);
    uart_write_char('\n');
    uart_write_char('\r');
    count += 1;

    P1OUT ^= redLED;

    // delay
    _delay_cycles(100000); // 100,000 cycles
  }
}
