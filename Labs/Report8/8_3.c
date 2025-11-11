#include <msp430fr6989.h>
#include <stdint.h>

#define redLED BIT0
#define FLAGS UCA1IFG
#define RXFLAG UCRXIFG
#define TXFLAG UCTXIFG
#define TXBUFFER UCA1TXBUF
#define RXBUFFER UCA1RXBUF

// 9600 baud based on 1 MHz SMCLK w/16x oversampling.
// 8 bits, no parity, LSB first, 1 stop bit UART communication
void Initialize_UART(void)
{
  // Configure pins to UART functionality
  P3SEL1 &= ~(BIT4 | BIT5);
  P3SEL0 |= (BIT4 | BIT5);
  // Main configuration register
  UCA1CTLW0 = UCSWRST;
  // Engage reset; change all the fields to zero
  // Most fields in this register, when set to zero, correspond to the
  // popular configuration
  UCA1CTLW0 |= UCSSEL_2; // Set clock to SMCLK
  // Configure the clock dividers and modulators (and enable oversampling)
  UCA1BRW = 6;
  // divider
  // Modulators: UCBRF = 8 = 1000--> UCBRF3 (bit #3)
  // UCBRS = 0x20 = 0010 0000 = UCBRS5 (bit #5)
  UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
  // Exit the reset state
  UCA1CTLW0 &= ~UCSWRST;
}

void Initialize_ADC(void)
{
  // Configure the pins to analog functionality
  // X-axis: A10/P9.2, for A10 (P9DIR=x, P9SEL1=1, P9SEL0=1)
  P9SEL1 |= BIT2;
  P9SEL0 |= BIT2;
  // Y-axis: A4/P8.7 (P8DIR=x, P8SEL1=1,P8SEL0=)
  P8SEL1 |= BIT7;
  P8SEL0 |= BIT7;
  // Turn on the ADC module
  ADC12CTL0 |= ADC12ON;
  // Turn off ENC (Enable Conversion) bit while modifying the configuration
  ADC12CTL0 &= ~ADC12ENC;
  //*************** ADC12CTL0 ***************
  // ADC12SHT0x sets SHT cycles for results 0-7, 24-31
  // ADC12MSC sets multiple analog inputs
  ADC12CTL0 |= ADC12SHT0_2; // Sets SHT of 16 cycles (found in doc. slau367o table 34.4)
  ADC12CTL0 |= ADC12MSC; // 1= multiple inputs
  //*************** ADC12CTL1 ***************
  // ADC12SHS sets read trigger
  // ADC12SHP sets SAMPCON use
  /// ADC12DIV sets clock divider
  // ADC12SSEL sets clock base
  // ADC12CONSEQx sets conversion sequence mode
  ADC12CTL1 |= ADC12SHS_0;  // 0 = ADC12SC bit
  ADC12CTL1 |= ADC12SHP;    // 1 = SAMPCON sourced from clock
  ADC12CTL1 |= ADC12DIV_0;  // 0 = /1
  ADC12CTL1 |= ADC12SSEL_0; // 0 = MODOSC
  ADC12CTL1 |= ADC12CONSEQ_1;
  // values in doc. slau367o table 34.5
  //*************** ADC12CTL2 ***************
  // ADC12RES sets bit resolution
  // ADC12DF sets data format
  ADC12CTL2 |= ADC12RES_2; // 2 = 12-bit
  ADC12CTL2 &= ~ADC12DF;   // 0 = unsigned binary
  //*************** ADC12CTL3 ***************
  // ADC12CSTARTADDx sets first ADC12MEM register in conversion sequence
  ADC12CTL3 |= ADC12CSTARTADD_0;
  //*************** ADC12MCTL0 ***************
  // ADC12VRSELx sets VR+ and VR- sources as well as buffering
  // ADC12INCHx sets analog input
  ADC12MCTL0 |= ADC12VRSEL_0; // 0 -> VR+ = AVCC and VR- = AVSS
  ADC12MCTL0 |= ADC12INCH_10; // 10 = A10 input
  //*************** ADC12MCTL1 ***************
  // ADC12ENC sets final conversion channel
  ADC12MCTL1 |= ADC12VRSEL_0;
  ADC12MCTL1 |= ADC12INCH_4;
  ADC12MCTL1 |= ADC12EOS; // 1 = last converted input
  // set ENC bit at end of config
  ADC12CTL0 |= ADC12ENC;
}

void uart_write_char(unsigned char ch)
{
  while ((FLAGS & TXFLAG) == 0)
  {
    // Wait for any ongoing transmission to complete
  }
  // Copy the byte to the transmit buffer
  TXBUFFER = ch; // Tx flag goes to 0 and Tx begins!
  return;
}

void uart_write_12bit(uint16_t n)
{
  const char hex_digits[] = "0123456789ABCDEF"; // Digits used in hexadecimal
  uint8_t digit;                                // one hex digit = 4 bits
  int i;
  // print the 0x part of hex format
  uart_write_char('0');
  uart_write_char('x');
  // Extract and print hex digits from input
  // i = 8 because bits 12-15 will always be 0000
  for (i = 8; i >= 0; i = i - 4)
  { 
    digit = (n >> i) & 0xF;
    uart_write_char(hex_digits[digit]);
  }
}

void main(void) 
{
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;

  P1DIR |= redLED;
  P1OUT |= redLED;

  Initialize_UART();
  Initialize_ADC();

  for (;;)
  {
    ADC12CTL0 |= ADC12SC; // Triggers ADC12BUSY while reading input
    while ((ADC12CTL1 & ADC12BUSY) != 0) 
    {
      // Wait for flag to drop
    } 
    ADC12CTL0 &= ~ADC12SC;
    uint16_t x_coord = ADC12MEM0; // ADC12MEM0 linked to A10, x-input
    uint16_t y_coord = ADC12MEM1; // ADC12MEM1 linked to A4, y-input
    uart_write_12bit(x_coord);    // Print x-coordinate to console
    uart_write_char(' ');         // Readability space
    uart_write_12bit(y_coord);    // Print y-coordinate to console
    uart_write_char('\n');        // newline
    P1OUT ^= redLED;              // toggle red LED
    _delay_cycles(500000);        // 0.5 second delay
  }
}

