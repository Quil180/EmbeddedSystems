#include <msp430.h>
#include <msp430fr6989.h>
#include <string.h>
#include <stdint.h>


#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer

#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7

#define BUT1 BIT1
#define BUT2 BIT2

// The array has the shapes of the digits (0 to 9)
// Complete this array...
const unsigned char LCD_Shapes[10] = {0xFC,0x60,0xDB,0xF3,0x67,0xB7,0xBF,0xE0, 0xFF, 0xF7} ;

volatile int number = 0;

volatile unsigned int LuxValue = 0;

volatile int time = 0;
volatile int current_time = 50000;


char HH_c[] = "00";
char MM_c[] = "00";
char SS_c[] = "00";

volatile int prevBase;

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
  for (i = 0; i < strlen(string); i++)
  {
    uart_write_char(string[i]);
  }
  return;
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

void uart_write_uint16 (uint16_t number)
{
  // Converting the number via snprintf
  char buffer[6]; // 5 characters is the max amount of characters for 65,536
  custom_itoa(number, buffer);
  uart_write_string(buffer);
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

// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal(void)
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

int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int *data)
{
  unsigned char byte1=0, byte2=0; // Intialize to ensure successful reading
  UCB1I2CSA = i2c_address; // Set address
  UCB1IFG &= ~UCTXIFG0;
  // Transmit a byte (the internal register address)
  UCB1CTLW0 |= UCTR;
  UCB1CTLW0 |= UCTXSTT;
  while(!(UCB1IFG & UCTXIFG0))
  {
    // Wait for flag to raise
  } 
  UCB1TXBUF = i2c_reg; // Write in the TX buffer
  while(!(UCB1IFG & UCTXIFG0))
  {
    // Buffer copied to shift register; Tx in progress; set Stop bit
  }
  // Repeated Start
  UCB1CTLW0 &= ~UCTR;
  UCB1CTLW0 |= UCTXSTT;
  // Read the first byte
  while(!(UCB1IFG & UCRXIFG0))
  {
    // Wait for flag to raise
  } 
  byte1 = UCB1RXBUF;
  // Assert the Stop signal bit before receiving the last byte
  UCB1CTLW0 |= UCTXSTP;
  // Read the second byte
  while(!(UCB1IFG & UCRXIFG0)) 
  {
    // Wait for flag to raise
  } 
  byte2 = UCB1RXBUF;
  while(UCB1CTLW0 & UCTXSTP) 
  {
  }
  while(UCB1STATW & UCBBUSY)
  {
  }
  *data = (byte1 << 8) | (byte2 & (unsigned int)0x00FF);
  return 0;
}

int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data)
{
  unsigned char byte1, byte2;

  UCB1I2CSA = i2c_address; // Set I2C address

  byte1 = (data >> 8) & 0xFF; // MSByte
  byte2 = data & 0xFF;        // LSByte

  UCB1IFG &= ~UCTXIFG0;

  // Write 3 bytes
  UCB1CTLW0 |= (UCTR | UCTXSTT);

  while(!(UCB1IFG & UCTXIFG0))
  {
    // Wait
  }
  UCB1TXBUF = i2c_reg;

  while(!(UCB1IFG & UCTXIFG0))
  {
    // Wait
  }
  UCB1TXBUF = byte1;

  while(!(UCB1IFG & UCTXIFG0))
  {
    // Wait
  }
  UCB1TXBUF = byte2;

  while(!(UCB1IFG & UCTXIFG0))
  {
    // Wait
  }

  UCB1CTLW0 |= UCTXSTP;
  while ( UCB1CTLW0 & UCTXSTP)
  {
    // Wait
  }
  while (UCB1STATW & UCBUSY)
  {
    // Wait
  }

  return 0;
}

void update_clock_numbers(unsigned int n)
{
  // A1 & A2 hours
  // A3 & A4 Mins
  // A5 & A6 seconds
  // assume numbers
  // divide by # secs in hours
  // divide by # secs in hours
  // divide by # 

  unsigned int HH = 0;
  unsigned int MM = 0;
  unsigned int SS = 0;

  unsigned int current_digit = 0;
  current_digit = n % 3600;    // gets current # hours

  // Seconds Logic 
  unsigned int seconds = n % 60;
  if (seconds > 0)
  {
    SS = seconds % 10;
  }
  if (seconds > 10)
  {
    SS += (seconds / 10) *10;
  }

  n /= 60;

  // Minutes Logic 
  unsigned int minutes = n % 60;
  if (minutes > 0)
  {
    // add the one digits to the thing
    MM += minutes % 10;
  }
  if (minutes > 10)
  {
    MM += (minutes / 10) * 10;
  }

  n = n/60;

  // Hours Logic
  unsigned int hours = n % (60);

  if (hours > 0)
  {
    HH += hours % 10;
  }
  if (hours > 10)
  {
    HH += (hours / 10) * 10;
  }

  // Printing
  uart_write_uint16(HH);
  uart_write_char(':');
  uart_write_uint16(MM);
  uart_write_char(':');
  uart_write_uint16(SS);
  uart_write_char('\t');
}

void main(void) 
{
  volatile int n;
  // Stop the Watchdog timer
  WDTCTL = WDTPW | WDTHOLD;

  // Unlock the GPIO pins
  PM5CTL0 &= ~LOCKLPM5;

  // Configure the LEDs as output
  P1DIR |= redLED; // Direct pin as output
  P1OUT &= ~redLED; // Turn LED Off

  P9DIR |= greenLED; // Direct pin as output
  P9OUT &= ~greenLED; // Turn LED Off

  //buttons
  P1DIR &= ~(BUT1 | BUT2);
  P1REN |=  (BUT1 | BUT2);
  P1OUT |=  (BUT1 | BUT2);

  P1IES |=  (BUT1 | BUT2); //1: Interrupt on falling edge (0 for rising edge)
  P1IFG &= ~(BUT1 | BUT2); //0: Clear the interrupt flags
  P1IE  |=  (BUT1 | BUT2); //1: Enable the interrupts
  
  config_ACLK_to_32KHz_crystal();

  // standard delay is 1 second with interrupts
  // Configure channel 0 for up mode with interrupts
  TA0CCR0 = 32768;    // 1 second @ 32kHz 
  TA0CCTL0 |= CCIE;   // Enable channel 0 CCIE1
  TA0CCTL0 &= ~CCIFG; // Clear Channel 0 CCIFG

  // Use ACLK, divide by 1, up mode, clear TAR 
  TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;

  // Ensure flag is cleared at the start
  TA0CTL &= ~TAIFG;
 
 // Enable Global Interrupt bit ( call an intrinsic function)
 _enable_interrupt();
 initialize_i2c();
 initialize_uart();

  i2c_write_word(0x44,0x01, 0x7604);
  
  P1OUT |= redLED;
  uart_write_string("done with init\n");
  _delay_cycles(600000);
  i2c_read_word(0x44, 0x00, &LuxValue);
  prevBase = LuxValue;

  for (;;)
  {   
    _delay_cycles(50000);
    // confirm loop in action
    P1OUT ^= redLED;
  } 
}

// one second Timer system
// interrupt for blinking
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TA00_ISR()
{
  P9OUT ^= greenLED;

  if (time == 1)
  {
    i2c_read_word(0x44, 0x00, &LuxValue);
    update_time();
    uart_write_char('\t');
    uart_write_uint16(LuxValue);
    uart_write_string(" lux");

    if (LuxValue > prevBase + 10)
    {
      uart_write_string("\t<Up>\n");
      prevBase = LuxValue;
    }
    else if (LuxValue < prevBase - 10)
    {
      uart_write_string("\t<Down>\n");
      prevBase = LuxValue;
    }
    else
    {
      uart_write_char('\n');
    }
    time = 0;
  }
  time = time + 1;
  current_time = current_time + 1;
}

void update_time()
{
  // if it is 9
  if (MM_c[1] == '9')
  {
    if (MM_c[0] < '5')
    {
      MM_c[0]++;
      MM_c[1] = '0';
    }
    else
    {
      //now increase hour by one
      MM_c[0] = '0';
      MM_c[1] = '0';
      // check last possible time, else go up
      if (HH_c[0] == '2' && HH_c[1] == '3')
      {
        HH_c[0] = '0';
        HH_c[1] = '0';
      }
      else if (HH_c[1] == '0')
      {
        HH_c[0]++;
        HH_c[1] = '0';
      }
      else
      {
        HH_c[1]++;
      }
    }
  }
  else
  {
    MM_c[1]++;
  }
  print_time();
}

void print_time()
{
  uart_write_string(HH_c);
  uart_write_char(':');
  uart_write_string(MM_c);
}

#pragma  vector = PORT1_VECTOR
__interrupt  void set_time()
{
  __delay_cycles(50000);
  if (P1IFG & BUT2) 
  {
    TA0CTL &= ~MC_3;
    uart_write_string("Enter the time...(3 or 4 digits then hit Enter)\n");
    char given_char;

    char time[] = "____";
    volatile int loc = 0;
    while (loc < 4)
    {
      given_char = uart_read_char();
      if (given_char ==0)
      {
        continue;
      }
      
      if (given_char == 3 || given_char == 27 || given_char == 13)
      {
        break;
      }
      time[loc] = given_char;
      loc++;
    } 
    uart_write_string("got out!\n");

    if (time[3] == 95)
    {
      uart_write_string("entered 3");
      HH_c[0] = 48;
      HH_c[1] = time[0];
      MM_c[0] = time[1];
      MM_c[1] = time[2];
    }
    else
    {
      uart_write_string("entered 4");
      HH_c[0] = time[0];
      HH_c[1] = time[1];
      MM_c[0] = time[2];
      MM_c[1] = time[3];
    }

    uart_write_string("Time set to ");
    print_time();
    uart_write_char('\n');

    P1IFG &= ~BUT2;
    TA0CTL |= MC_1;
  }
}
