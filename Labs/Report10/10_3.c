// Part 9.3 Code ---------------------------
#include "Grlib/grlib/grlib.h"    // Graphics library (grlib)
#include "LcdDriver/lcd_driver.h" // LCD driver
#include "msp430fr6989.h"
#include <math.h>
#include <stdio.h>

#define redLED BIT0
#define greenLED BIT7
#define S1 BIT1
#define S2 BIT2

// For I2C
#define MANUFACTURERIDREG 0x7E // Manufacturer Register ID
#define DEVICEIDREG 0x7F       // Device Register ID
#define I2CADDR 0x44           // I2C Address

// For reading Sensor data
#define CONFIGREG 0x01
#define RESULTREG 0x00

void Initialize_Clock_System() 
{
  // DCO frequency = 16 MHz
  // MCLK = fDCO/1 = 16 MHz
  // SMCLK = fDCO/1 = 16 MHz
  // Activate memory wait state
  FRCTL0 = FRCTLPW | NWAITS_1; // Wait state=1
  CSCTL0 = CSKEY;
  // Set DCOFSEL to 4 (3-bit field)
  CSCTL1 &= ~DCOFSEL_7;
  CSCTL1 |= DCOFSEL_4;
  // Set DCORSEL to 1 (1-bit field)
  CSCTL1 |= DCORSEL;
  // Change the dividers to 0 (div by 1)
  CSCTL3 &= ~(DIVS2 | DIVS1 | DIVS0); // DIVS=0 (3-bit)
  CSCTL3 &= ~(DIVM2 | DIVM1 | DIVM0); // DIVM=0 (3-bit)
  CSCTL0_H = 0;
  return;
}

// I2C Initialization Function (for lux sensor)
void Initialize_I2C(void)
{
  // Configure the MCU in Master mode
  // Configure pins to I2C functionality
  // (UCB1SDA same as P4.0) (UCB1SCL same as P4.1)
  // (P4SEL1=11, P4SEL0=00) (P4DIR=xx)
  P4SEL1 |= (BIT1 | BIT0);
  P4SEL0 &= ~(BIT1 | BIT0);
  // Enter reset state and set all fields in this register to zero
  UCB1CTLW0 = UCSWRST;
  // Fields that should be nonzero are changed below
  // (Master Mode: UCMST) (I2C mode: UCMODE_3) 
  // (Synchronous mode: UCSYNC)
  // (UCSSEL 1:ACLK, 2,3:SMCLK)
  UCB1CTLW0 |= UCMST | UCMODE_3 | UCSYNC | UCSSEL_3;
  // Clock frequency: 16 MHz, 16M/320K =
  UCB1BRW = 50;
  // Chip Data Sheet p. 53 (Should be 400 KHz max)
  // Exit the reset mode at the end of the configuration
  UCB1CTLW0 &= ~UCSWRST;
}

// Read a word (2 bytes) from I2C (address, register)
int i2c_read_word(unsigned char i2c_address,
                  unsigned char i2c_reg,
                  unsigned int *data)
{
  // Intialize to ensure successful
  unsigned char byte1 = 0;
  unsigned char byte2 = 0; 
  reading UCB1I2CSA = i2c_address;    // Set address
  UCB1IFG &= ~UCTXIFG0;
  // Transmit a byte (the internal register address)
  UCB1CTLW0 |= UCTR;
  UCB1CTLW0 |= UCTXSTT;
  while ((UCB1IFG & UCTXIFG0) == 0) 
  {
    // Wait for flag to raise
  } 
  UCB1TXBUF = i2c_reg; // Write in the TX buffer
  while ((UCB1IFG & UCTXIFG0) == 0) 
  {
    // Buffer copied to shift
  } 
  register;
  // Tx in progress; set Stop bit
  // Repeated Start
  UCB1CTLW0 &= ~UCTR;
  UCB1CTLW0 |= UCTXSTT;
  // Read the first byte
  while ((UCB1IFG & UCRXIFG0) == 0)
  {
    // Wait for flag to raise
  }
  byte1 = UCB1RXBUF;
  // Assert the Stop signal bit before receiving the last byte
  UCB1CTLW0 |= UCTXSTP;
  // Read the second byte
  while ((UCB1IFG & UCRXIFG0) == 0) 
  {
    // Wait for flag to raise
  }
  byte2 = UCB1RXBUF;
  while ((UCB1CTLW0 & UCTXSTP) != 0) 
  {
    // wait
  }
  while ((UCB1STATW & UCBBUSY) != 0) 
  {
    // wait
  }
  *data = (byte1 << 8) | (byte2 & (unsigned int)0x00FF);
  return 0;
}

// Write a word (2 bytes) to I2C (address, register)
int i2c_write_word(unsigned char i2c_address, 
                   unsigned char i2c_reg,
                   unsigned int data)
{
  unsigned char byte1, byte2;
  UCB1I2CSA = i2c_address;    // Set I2C address
  byte1 = (data >> 8) & 0xFF; // MSByte
  byte2 = data & 0xFF;        // LSByte
  UCB1IFG &= ~UCTXIFG0;
  // Write 3 bytes
  UCB1CTLW0 |= (UCTR | UCTXSTT);
  while ((UCB1IFG & UCTXIFG0) == 0) 
  {
  }
  UCB1TXBUF = i2c_reg;
  while ((UCB1IFG & UCTXIFG0) == 0) 
  {
  }
  UCB1TXBUF = byte1;
  while ((UCB1IFG & UCTXIFG0) == 0) 
  {
  }
  UCB1TXBUF = byte2;
  while ((UCB1IFG & UCTXIFG0) == 0)
  {
  }
  UCB1CTLW0 |= UCTXSTP;
  while ((UCB1CTLW0 & UCTXSTP) != 0) 
  {
  }
  while ((UCB1STATW & UCBBUSY) != 0)
  {
  }
  return 0;
}

// Initializing Sensor
void Initialize_SENS() 
{
  // Configuration for Sensor based on lab manual
  unsigned volatile int configuration_value = 0x7604;
  i2c_write_word(I2CADDR, CONFIGREG, configuration_value);
}

void main(void) 
{
  char mystring[20];
  // Configure WDT & GPIO
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;
  // Configure LEDs
  P1DIR |= redLED;
  P9DIR |= greenLED;
  P1OUT &= ~redLED;
  P9OUT &= ~greenLED;
  // Configure buttons
  P1DIR &= ~(S1 | S2);
  P1REN |= (S1 | S2);
  P1OUT |= (S1 | S2);
  P1IFG &= ~(S1 | S2); // Flags are used for latched polling
  // Set the LCD backlight to highest level
  // P2DIR |= BIT6;
  // P2OUT |= BIT6;

  // Configure clock system
  Initialize_Clock_System();

  // Initializing the I2C
  Initialize_I2C();

  // Initializing Sensor with new function based on configuration
  Initialize_SENS();

  // Graphics functions
  Graphics_Context g_sContext;        // Declare a graphic library
  context Crystalfontz128x128_Init(); // Initialize the display
  // Set the screen orientation
  Crystalfontz128x128_SetOrientation(0);
  // Initialize the context
  Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
  // Set background and foreground colors
  Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
  Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
  // Set the default font for strings
  GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
  // Clear the screen
  Graphics_clearDisplay(&g_sContext);

  // Set up Optical Counter Display
  sprintf(mystring, "Optical Counter");
  // show "Optical Sensor"
  Graphics_drawStringCentered(&g_sContext,
                              mystring,
                              AUTO_STRING_LENGTH,
                              64, 
                              40,
                              OPAQUE_TEXT);
  Graphics_setForegroundColor(&g_sContext,
                              GRAPHICS_COLOR_GOLDENROD);
  sprintf(mystring, "lux");
  // show "lux"
  Graphics_drawStringCentered(&g_sContext,
                              mystring, 
                              AUTO_STRING_LENGTH, 
                              90, 
                              80,
                              OPAQUE_TEXT);
  // Make rectangular outline for optical sensor value
  Graphics_setForegroundColor(&g_sContext,
                              GRAPHICS_COLOR_GOLDENROD);
  const Graphics_Rectangle RectLoc = {
      .xMin = 10, .yMin = 100,
      .xMax = (128 - 10), .yMax = (100 + 10)
  };
  Graphics_drawRectangle(&g_sContext, &RectLoc);
  int LUX_Val = 0;
  int Object_Counter = 0;
  char str[5];
  bool update_object_num = 0;
  while (1) 
  {
    // Storing sensor value based on value in result
    // reg and lux reading from sensor
    i2c_read_word(I2CADDR, RESULTREG, &LUX_Val);
    // make counter purple
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    LUX_Val = LUX_Val * 1.28;
    if (LUX_Val >= 1000) 
    {
      LUX_Val = 1000; // Caps LUX_Val at 1000 (based on manual)
    }
    if ((LUX_Val <= 200) && (update_object_num == 0)) 
    {
      // For object, count the object
      detection Object_Counter++;
      update_object_num = 1;
    } 
    else if ((LUX_Val > 200) && (update_object_num == 1)) 
    {
      // Won't update again until object passes
      update_object_num = 0;
    }
    sprintf(str, "%d", Object_Counter); // Convert number to string
    Graphics_drawStringCentered(&g_sContext,
                                str, 
                                AUTO_STRING_LENGTH, 
                                64, 
                                60,
                                OPAQUE_TEXT); // Display number
    // Convert number to string
    sprintf(str, "%4d", LUX_Val);
    Graphics_drawStringCentered(&g_sContext,
                                str,
                                AUTO_STRING_LENGTH,
                                62, 
                                80,
                                OPAQUE_TEXT); // Display number
    __delay_cycles(500000);                   // wait a second
    // Make rectangular outline for showing optical sensor value
    if (round(LUX_Val / 9.434) + 1 < 105) 
    {
      Graphics_setForegroundColor(&g_sContext,
                                  GRAPHICS_COLOR_BLACK);
      const Graphics_Rectangle RectLoc = {
        .xMin = 11 + round(LUX_Val / 9.434) + 1,
        .yMin = 101,
        .xMax = (128 - 11),
        .yMax = (100 + 9)
      };
      Graphics_fillRectangle(&g_sContext, &RectLoc);
    }
    // Make rectangular outline for showing optical sensor value
    Graphics_setForegroundColor(&g_sContext,
                                GRAPHICS_COLOR_GOLDENROD);
    const Graphics_Rectangle RectLoc1 = {
      .xMin = 11,
      .yMin = 101,
      .xMax = 11 + round(LUX_Val / 9.434),
      .yMax = (100 + 9)
    };
    Graphics_fillRectangle(&g_sContext, &RectLoc1);
  }
}
