#include <msp430fr6989.h>
#include <stdint.h>
#include <string.h>

// LED BITS
#define red BIT0
#define green BIT7

// Button BITS
#define but1 BIT1
#define but2 BIT2

// UART Channels are P3.4 and P3.5 for transmit and recieve respectively
#define transmit BIT4
#define recieve BIT5

// WE LOVE DEFINES
#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer

// Global variables for states of runway1 and 2
volatile int red_state = 0; // runway 1 state
volatile int green_state = 0; // runway 2 state
volatile int blink_state = 0;

// Reverses a given string
void strrev(char *str) 
{
  int i = 0;
  int j = strlen(str) - 1;
  char temp;
  while (i < j) 
  {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++;
    j--;
  }
}

// Converts an unsigned 16-bit integer to a null-terminated string (base 10).
void custom_itoa(uint16_t number, char *buffer)
{
  int i = 0;

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
  strrev(buffer);
}

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

unsigned char uart_read_char(void)
{
  if (!(FLAGS & RXFLAG))
  {
    return 0; // no byte was recieved
  }

  // Return the buffer
  volatile unsigned char return_char = RXBUFFER;
  return return_char;
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

void uart_write_uint16 (uint16_t number)
{
  // Converting the number via snprintf
  char buffer[6]; // 5 characters is the max amount of characters for 65,536
  custom_itoa(number, buffer);
  uart_write_string(buffer);
  return;
}

int main(void)
{
  // Enabling the leds and other stuff
  WDTCTL   = WDTPW | WDTHOLD; // Stop WDT
  PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO pins

  // Configuring the LEDs as outputs and off
  P1DIR |= red;
  P9DIR |= green;
  P1OUT &= ~red;
  P9OUT &= ~green;

  // Defining the buttons as inputs and their resistors as pull-up
  P1DIR &= ~(but1 | but2);
  P1REN |= (but1 | but2);
  P1OUT |= (but1 | but2);

  // Setting up button interrupts on FALLING EDGE
  // P1IES |= (but1 | but2);
  P1IE  |= (but1 | but2);
  P1IFG &= ~(but1 | but2);

  // Setting ACLK to 32KHz
  config_ACLK_to_32KHz_crystal();

  // Setting up the timer
  TA0CCR0   = 32767;  // 1 second interrupts
  TA0CCTL0 = CCIE;   // Enabling capture control interrupt
  TA0CCTL0 &= ~CCIFG; // Clearing interrupt flag (if there)
  // ACLK is used, no divider, up mode, and clear the current stored time
  TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
  // Sanity check for timer_a interrupt flag
  TA0CTL &= ~TAIFG;

  // doing what the function says
  initialize_uart(); 

  // Since we aren't using low power mode
  _enable_interrupt();

  // I think you'll know what this does...
  uart_write_string("\033[m\033[1;1H\033[2J"
                          "ORLANDO EXECUTIVE AIRPORT RUNWAY CONTROL\n"
                          "\n"
                          "\n"
                          "                 Runway 1    Runway 2\n"
                          "Request (RQ):       1           3\n"
                          "Forfeit (FF):       7           9\n"
                          "\n"
                          "\n"
                          "\n"
                          "--------                         --------\n"
                          "RUNWAY 1                         RUNWAY 2\n"
                          "--------                         --------\n"
                          "\n"
                          "\n"
                          "\n"
                          "\n"
                          );

  // Actual logic for selection
  for (;;)
  { 
    unsigned char given_char = uart_read_char();
    
    switch(given_char) 
    {
      case '1':
        uart_write_string("\0337\033[13;HRequested\0338");
        P1OUT |= red;
        red_state = 1;
        break;
      case '3':
        uart_write_string("\0337\033[13;34HRequested\0338");
        P9OUT |= green;
        green_state = 1;
        break;
      case '7':
        uart_write_string("\0337\033[13;H                 \033[14;H                 \033[16;H                 \0338");
        P1OUT &= ~red;
        red_state = 0;
        blink_state = 0;
        break;
      case '9':
        uart_write_string("\0337\033[13;34H                 \033[14;34H                 \033[16;34H                 \0338");
        P9OUT &= ~green;
        green_state = 0;
        blink_state = 0;
        break;
    }

    __delay_cycles(25000);
  }
}

// button interrupts
#pragma vector = PORT1_VECTOR
__interrupt void P1_ISR()
{
  if (P1IFG & but1)
  {
    if (red_state)
    {
      if(blink_state == 3 || blink_state == 4)
      {
        // Do Nothing
      }
      else if (blink_state == 2)
      {
        blink_state = 1;
      }
      else
      {
        blink_state = 2;
      }
    }
    P1IFG &= ~but1;
  }

  if (P1IFG & but2)
  {
    if (green_state)
    {
      if(blink_state == 1 || blink_state == 2)
      {
        // Do Nothing
      }
      else if (blink_state == 4)
      {
        blink_state = 3;
      }
      else
      {
        blink_state = 4;
      }
    }
    P1IFG &= ~but2;
  }

  __delay_cycles(200000); // 200 MS
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void TA0_ISR()
{
  switch (blink_state)
  {
    case 1:
      uart_write_string("\0337\033[16;H*** Inquiry ***      \0338");
      // We then need to do #2
    case 2:
      P1OUT ^= red;
      uart_write_string("\0337\033[14;HIn Use       \033[14;34H             \0338");
      if (green_state)
      {
        P9OUT |= green;
      }
      else
      {
        P9OUT &= ~green;
      }
      break;
    case 3:
      uart_write_string("\0337\033[16;34H*** Inquiry ***      \0338");
      // We need to do #3
    case 4:
      P9OUT ^= green;
      uart_write_string("\0337\033[14;34HIn Use       \033[14;H             \0338");
      if (red_state)
      {
        P1OUT |= red;
      }
      else
      {
        P1OUT &= ~red;
      }
      break;
  }
}
