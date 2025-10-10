#include <msp430fr6989.h>
#include <stdbool.h>
#include <stdint.h>

#define red BIT0   // Port 1.0
#define green BIT7 // Port 9.7

#define but1 BIT1 // Port 1.1
#define but2 BIT2 // Port 1.2

bool state = true; // true is unpaused
volatile uint16_t totalSec = 0;
volatile uint16_t seconds = 0;
volatile uint16_t minutes = 0;
volatile uint16_t hours = 0;
volatile uint16_t counter = 0;
uint16_t holding = 1;


const unsigned char LCD_Shapes[10] = { 0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE0, 0xFF, 0xE7 };
uint16_t digits[6] = {0};

int checkingBut2And1 = 0;

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

void toggle_colon()
{
  // All this function does is toggle the colon
  LCDM7 ^= BIT2;
}

void toggle_exclamation()
{
  LCDM3 |= BIT0;
  LCDM3 &= ~BIT3;
}

void toggle_stopwatch()
{
  LCDM3 |= BIT3;
  LCDM3 &= ~BIT0;
}

// Initializes the LCD_C module
void Initialize_LCD() 
{
  PJSEL0 = BIT4 | BIT5; // For LFXT
  LCDCPCTL0 = 0xFFD0;
  LCDCPCTL1 = 0xF83F;
  LCDCPCTL2 = 0x00F8;
  // Configure LFXT 32kHz crystal
  CSCTL0_H = CSKEY >> 8;
  CSCTL4 &= ~LFXTOFF;
  do 
  {
    // Unlock CS registers
    // Enable LFXT
    CSCTL5 &= ~LFXTOFFG;
    SFRIFG1 &= ~OFIFG;
  } 
  while (SFRIFG1 & OFIFG);
  CSCTL0_H = 0; // Clear LFXT fault flag
  // Test oscillator fault flag
  // Lock CS registers
  // Initialize LCD_C
  // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
  LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;
  // VLCD generated internally,
  // V2-V4 generated internally, v5 to ground
  // Set VLCD voltage to 2.60v
  // Enable charge pump and select internal reference for it
  LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
  LCDCCPCTL = LCDCPCLKSYNC; // Clock synchronization enabled
  LCDCMEMCTL = LCDCLRM; // Turn LCD on (do this at the end!)
  LCDCCTL0 |= LCDON;
  // Clear LCD memory
  return;
}

// This function does not initialize the timer, but instead is used for starting and stopping it.
// This function returns the time that was inputted if succesfull, and returns -1 if unsuccessful
int startTime(uint16_t time_given) 
{
  if (time_given < 0)
  {
    // Returning false.
    return -1;
  }

  // time_given is a valid value
  TA0CCR0 = time_given;
  TA0CCTL0 = CCIE;
  TA0CCTL0 &= ~CCIFG;
  TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
  TA0CTL &= ~TAIFG; // Clearing interrupt flag (if any)

  // Returning success
  return time_given;
}

// This function initializes the timer to work.
void initializeTime()
{
  TA0CTL = TASSEL_1 | ID_0 | MC_2 | TACLR | TAIE;
  TA0CTL &= ~TAIFG; // Clearing any previous flags (if any)
}

void stopTime()
{
  // Clearing the timer.
  TA0CTL = MC_0 | TACLR;
}

void LCDframe()
{
  // Enable the decimal point
  LCDM20 = BIT0;

  if (state)
  {
    // we are playing time
    toggle_stopwatch();
    TA0CTL |= MC_1;
    if (totalSec > 43199)
    {
      totalSec = 0;
    }
  }
  else
  {
    toggle_exclamation();
    TA0CTL &= ~MC_3;
  }
  toggle_colon();

  seconds = totalSec % 60;
  minutes = (totalSec % 3600) / 60;
  hours = totalSec / 3600;

  uint8_t i = 0; // temporary counter

  uint8_t *display[6] = {&LCDM10, &LCDM6, &LCDM4, &LCDM19, &LCDM15, &LCDM8};

  uint16_t temp;

  temp = seconds;
  // Writing seconds
  for (i = 0; i < 2; i++)
  {
    digits[i] = temp % 10;
    temp /= 10; // go down
    *display[5 - i] = LCD_Shapes[digits[i]];
  }

  // Writing minutes
  temp = minutes;
  for (i = 0; i < 2; i++)
  {
    digits[i] = temp % 10;
    temp /= 10;
    *display[3 - i] = LCD_Shapes[digits[i]];
  }

  // Writing hours
  temp = hours;
  for (i = 0; i < 2; i++)
  {
    digits[i] = temp % 10;
    temp /= 10;
    *display[1 - i] = LCD_Shapes[digits[i]];
  }
}

int main(void) 
{
  WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
  PM5CTL0 &= ~LOCKLPM5;     // Enable the GPIO pins

  P1DIR |= red;  // Set output for red led
  P1OUT &= ~red; // Turn off red LED

  P9DIR |= green;  // Set output for green led
  P9OUT &= ~green; // Turn off green LED

  // Using default initialize function
  Initialize_LCD();
  LCDCMEMCTL = LCDCLRM; // Clear LCD

  // Configure buttons
  P1DIR &= ~(but1 | but2); // input
  P1REN |= (but1 | but2);  // enable resistors
  P1OUT |= (but1 | but2);  // pull-up

  P1IES |= (but1 | but2);   // interrupt on falling edge
  P1IE  |= (but1 | but2);   // enable interrupts
  P1IFG  = 0;               // set interrupt flag off

  config_ACLK_to_32KHz_crystal();

  // initializeTime(); // To set the interrupt flag forever.
  startTime(32767); // Clock max is 1 second per.

  _low_power_mode_0();

  return 0;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR() 
{
  if ((P1IFG & but1))
  {
    holding = 1;
    // S1 was pressed

    while (!(P1IN & but1))
    {
      // S1 was held.
      holding += 1;
      _delay_cycles(10000);
      if (holding > 110)
      {
        break;
      }
    }

    if (holding > 100) 
    {
      // We held it long enough.
      state = false;
      totalSec = 0;
      // Stop the state.
    }
    else
    {
      state = !state;
    }

    P1IFG &= ~but1; // clear flag
    LCDframe();
      __delay_cycles(50000); // 50k cycles for debouncing
  }

  if ((P1IFG & but2))
  {
    checkingBut2And1 = 1;
    while (!(P1IN & but2))
    {
      LCDframe();
      // You are pushing down only button2
      if (!(P1IN & but1))
      {
        totalSec--;
      }
      else 
      {
        totalSec++;
      }
      __delay_cycles(100); // debounce
    }
    P1IFG &= ~but2;
  }
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR()
{
  // generate a new frame every time the timer overflows.
  totalSec += 1;
  LCDframe();
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void T0A1_ISR()
{
  // Do nothing
}
