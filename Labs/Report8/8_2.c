#include <msp430fr6989.h>
#include <stdint.h>
#include <string.h>

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

// Functions provided by the lab
void initialize_i2c(void)
{
  // Configure the MCU in Master mode
  // Configure pins to I2C functionality
  // (UCB1SDA same as P4.0) (UCB1SCL same as P4.1)
  // (P4SEL1=11, P4SEL0=00) (P4DIR=xx)
  P4SEL1 |= (BIT1|BIT0);
  P4SEL0 &= ~(BIT1|BIT0);
  // Enter reset state and set all fields in this register to zero
  UCB1CTLW0 = UCSWRST;
  // Fields that should be nonzero are changed below
  // (Master Mode: UCMST) (I2C mode: UCMODE_3) (Synchronous mode: UCSYNC)
  // (UCSSEL 1:ACLK, 2,3:SMCLK)
  UCB1CTLW0 |= UCMST | UCMODE_3 | UCSYNC | UCSSEL_3;
  // Clock frequency: SMCLK/8 = 1 MHz/8 = 125 KHz
  UCB1BRW = 8;
  // Chip Data Sheet p. 53 (Should be 400 KHz max)
  // Exit the reset mode at the end of the configuration
  UCB1CTLW0 &= ~UCSWRST;
}

int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data)
{
  unsigned char byte1=0, byte2=0; // Intialize to ensure successful reading
  UCB1I2CSA = i2c_address; // Set address
  UCB1IFG &= ~UCTXIFG0;
  // Transmit a byte (the internal register address)
  UCB1CTLW0 |= UCTR;
  UCB1CTLW0 |= UCTXSTT;
  while((UCB1IFG & UCTXIFG0)==0) {} // Wait for flag to raise
  UCB1TXBUF = i2c_reg; // Write in the TX buffer
  while((UCB1IFG & UCTXIFG0)==0) {} // Buffer copied to shift register; Tx in progress; set Stop bit
  // Repeated Start
  UCB1CTLW0 &= ~UCTR;
  UCB1CTLW0 |= UCTXSTT;
  // Read the first byte
  while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
  byte1 = UCB1RXBUF;
  // Assert the Stop signal bit before receiving the last byte
  UCB1CTLW0 |= UCTXSTP;
  // Read the second byte
  while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
  byte2 = UCB1RXBUF;
  while((UCB1CTLW0 & UCTXSTP)!=0) {}
  while((UCB1STATW & UCBBUSY)!=0) {}
  *data = (byte1 << 8) | (byte2 & (unsigned int)0x00FF);
  return 0;
}

int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data)
{
  unsigned char byte1, byte2;

  UCB1I2CSA = i2c_address;             // Set I2C address

  byte1 = (data >> 8) & 0xFF;          // MSByte
  byte2 = data & 0xFF;                 // LSByte

  UCB1IFG &= ~UCTXIFG0;

  // Write 3 bytes
  UCB1CTLW0 |= (UCTR | UCTXSTT);

  while( (UCB1IFG & UCTXIFG0) == 0) {}
  UCB1TXBUF = i2c_reg;

  while( (UCB1IFG & UCTXIFG0) == 0) {}
  UCB1TXBUF = byte1;

  while( (UCB1IFG & UCTXIFG0) == 0) {}
  UCB1TXBUF = byte2;

  while( (UCB1IFG & UCTXIFG0) == 0) {}

  UCB1CTLW0 |= UCTXSTP;
  while( (UCB1CTLW0 & UCTXSTP) != 0 ) {}
  while((UCB1STATW & UCBUSY)!=0) {}

  return 0;
}

// Reverses a given string
void strrev(char *str)
{
  unsigned int i = 0;
  unsigned int j = strlen(str) - 1;
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
  unsigned int i; // counter
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

void uint16_to_4hex(unsigned int given_uint, char output[5])
{
  static const char hex[] = "0123456789ABCDEF"; // All possible hexes
  output[0] = hex[(given_uint >> 12) & 0xF];
  output[1] = hex[(given_uint >> 8) & 0xF];
  output[2] = hex[(given_uint >> 4) & 0xF];
  output[3] = hex[(given_uint >> 12) & 0xF];
  output[4] = hex[(given_uint) & 0xF];
  return;
}

int main(void)
{
  // Enabling the leds and other stuff
  WDTCTL   = WDTPW | WDTHOLD; // Stop WDT
  PM5CTL0 &= ~LOCKLPM5;       // Enable GPIO pins

  // doing what the function says
  initialize_uart();

  // yup, whatever it says
  initialize_i2c();

  // Actual logic for selection
  for (;;)
  {
    unsigned int light = 0;

    // Writing the configuration to the light sensor
    /*
    *  The configuration register is:
    *  RN [15:12] (R/W)- b1100 is reset
    *  CT [11] (R/W)- b1 is reset
    *  M[ 10:9] (R/W) - b00 is reset
    *  OVF [8] (R) - b0 is reset
    *  CRF [7] (R) - b0 is reset
    *  FH [6] (R) - b0 is reset
    *  FL [5] (R) - b0 is reset
    *  L [4] (R/W) - b1 is reset
    *  POL [3] (R/W) - b0 is reset
    *  ME [2] (R/W) - b0 is reset
    *  FC [1:0] (R/W) - b00 is reset
    *
    *  R/W is Read/Write
    *  R is Read
    */
    i2c_write_word(0x44, 0x01, 0x7614);
    // Reading the value of the light sensor
    i2c_read_word(0x44, 0x00, &light);

    // Converting the gathered reading to the proper value
    int correctedLight = light * 1.28;

    // writing to the serial console what the light sensor found
    uart_write_string("Lux: ");
    uart_write_uint16(correctedLight);
    uart_write_char('\n');

    __delay_cycles(1000000); // delay of 1 million cycles
  }
}

