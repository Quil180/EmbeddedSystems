#include <msp430fr6989.h>
#include <stdint.h>

#define redLED BIT0 // Red at P1.0
#define greenLED BIT7 // Green at P9.7

#define but1 BIT1 // Port 1.1
#define but2 BIT2 // Port 1.2

// The array has the shapes of the digits (0 to 9)
const unsigned char LCD_Shapes[10] = { 0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE0, 0xFF, 0xE7 };
uint16_t counter = 0;

//**********************************************************
// Initializes the LCD_C module
void Initialize_LCD() 
{
  PJSEL0 = BIT4 | BIT5; // For LFXT
  LCDCPCTL0 = 0xFFD0;
  LCDCPCTL1 = 0xF83F;
  LCDCPCTL2 = 0x00F8;
  // Configure LFXT 32kHz crystal
  CSCTL0_H = CSKEY >> 8; // Unlock CS registers
  CSCTL4 &= ~LFXTOFF; // Enable LFXT
  do 
  {
    CSCTL5 &= ~LFXTOFFG; // Clear LFXT fault flag
    SFRIFG1 &= ~OFIFG;
  }
  while (SFRIFG1 & OFIFG); // Test oscillator fault flag
  CSCTL0_H = 0; // Lock CS registers
  // Initialize LCD_C
  // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
  LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;
  // VLCD generated internally,
  // V2-V4 generated internally, v5 to ground
  // Set VLCD voltage to 2.60v
  // Enable charge pump and select internal reference for it
  LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
  LCDCCPCTL = LCDCPCLKSYNC; // Clock synchronization enabled
  LCDCMEMCTL = LCDCLRM; // Clear LCD memory
  //Turn LCD on (do this at the end!)
  LCDCCTL0 |= LCDON;
  return;
}

void lcd_write_uint16 (uint16_t number)
{
  uint8_t digits[5]; // we have a max of 5 possible digits
  // Addresses for each possible digit.
  volatile unsigned char *display[5] = { &LCDM6, &LCDM4, &LCDM19, &LCDM15, &LCDM8 };

  uint8_t i = 0;
  for (i = 0; i < 5; i++)
  {
    // Finding all digits in the number inputted
    digits[4 - i] = number % 10;
    number /= 10;
  }

  int leading = 0;
  for (i = 0; i < 5; i++)
  {
    // Incrementing through all 5 digits
    if (digits[i])
    {
      // A digit exists at this display section
      leading = 1;
    }
    if ((i == 4) && (leading == 0))
    {
      // We don't have any value at this display section
      *display[i] = LCD_Shapes[0];
    }
    else
    {
      // The digit we are on was not the 5th one OR it was the 5th
      // one and was not a leading digit
      if (leading)
      {
        // We have a leading digit
        *display[i] = LCD_Shapes[digits[i]];
      }
      else 
      {
        // We do not have a leading digit
        *display[i] = 0;
      }
    }
  }

  // Exiting function
  return;
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

int main(void)
{
  volatile unsigned int n;
  WDTCTL = WDTPW | WDTHOLD; // Stop WDT
  PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins
  P1DIR |= redLED; // Pins as output
  P9DIR |= greenLED;
  P1OUT |= redLED; // Red on
  P9OUT &= ~greenLED; // Green off

  P1DIR &= ~(but1 | but2);
  P1REN |= (but1 | but2);
  P1OUT |= (but1 | but2);
  P1IES |= (but1 | but2);
  P1IFG &= (but1 | but2);
  P1IE  |= (but1 | but2);
  P1IFG = 0; // resetting flag for interrupt

  // Initializes the LCD_C module
  Initialize_LCD();

  config_ACLK_to_32KHz_crystal();

  // Starting timer
  TA0CCR0 = 32767;
  TA0CCTL0 = CCIE;
  TA0CCTL0 &= ~CCIFG;
  TA0CTL = TASSEL_1 | ID_0 | MC_1 | TAIE;
  TA0CTL &= ~TAIFG;

  _low_power_mode_3();

  return 0;
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void TA0_ISR()
{
  lcd_write_uint16(counter);
  counter += 1;
  TA0CTL &= ~TAIFG;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR() 
{
  if ((P1IFG & but1))
  {
    counter = 0;
    P1IFG &= ~but1; // clear flag
  }

  if ((P1IFG & but2))
  {
    counter += 1000;
    P1IFG &= ~but2; // clear flag
  }
}
