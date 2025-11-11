// Part 9.2 Code -----------------------------
#include "Grlib/grlib/grlib.h"    // Graphics library (grlib)
#include "LcdDriver/lcd_driver.h" // LCD driver
#include "msp430fr6989.h"

#include <stdio.h>
#define redLED BIT0
#define greenLED BIT7
#define S1 BIT1
#define S2 BIT2

extern const tImage UCF_Logo;

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

  // Graphics functions
  Graphics_Context g_sContext;        // Declare a graphic library
  context Crystalfontz128x128_Init(); // Initialize the display
  // Set the screen orientation
  Crystalfontz128x128_SetOrientation(0);
  // Initialize the context
  Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
  // 1/2) Set new background and foreground colors
  Graphics_setBackgroundColor(
      &g_sContext, GRAPHICS_COLOR_BLACK); // Set new background color to white
  Graphics_setForegroundColor(
      &g_sContext, GRAPHICS_COLOR_PINK); // Set new foreground color to blue
  while (1) {
    // FOR IMAGE DISPLAY (DISPLAY 2)
    // 1/2) Set new background and foreground colors
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    // Set new background color to white
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLD);
    // Set new foreground color to blue
    // Clear the screen
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawImage(&g_sContext, &UCF_Logo, 1, 1);
    while ((P1IN & S1) != 0)
    {
      // wait
    }
    __delay_cycles(8000000);
    // FOR COUNTER AND SHAPES DISPLAY (DISPLAY 1)
    // 1/2) Set new background and foreground colors
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    // Set new background color to white
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
    // Set new foreground color to blue
    // Set the default font for strings
    GrContextFontSet(&g_sContext, &g_sFontlucidasans8x15);
    // Clear the screen
    Graphics_clearDisplay(&g_sContext);
    // 3) Draw an outline circle
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    // make circle outline red
    Graphics_drawCircle(&g_sContext, 25, 20, 10);
    // 3) Draw a filled circle
    Graphics_setForegroundColor(
        &g_sContext,
        GRAPHICS_COLOR_DARK_ORANGE); // make filled circle orange
    Graphics_fillCircle(&g_sContext, 50, 20, 10);
    // 3) Draw an outline rectangle
    Graphics_setForegroundColor(
        &g_sContext,
        GRAPHICS_COLOR_YELLOW); // make rectangle outline yellow
    const Graphics_Rectangle RectLoc = {
        .xMin = 15, .yMin = 40, .xMax = 35, .yMax = 65
    };
    Graphics_drawRectangle(&g_sContext, &RectLoc);
    // 3) Draw a filled rectangle
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
    // make filled rectangle green
    const Graphics_Rectangle RectLoc1 = {
        .xMin = 40, .yMin = 40, .xMax = 60, .yMax = 65};
    Graphics_fillRectangle(&g_sContext, &RectLoc1);
    // 3) Draw a horizontal line
    Graphics_setForegroundColor(
        &g_sContext,
        GRAPHICS_COLOR_DODGER_BLUE); // make horizontal line blue
    Graphics_drawLineH(&g_sContext, 15, 60, 80);
    // 4) Display counter
    // Set the default font for strings
    Graphics_setForegroundColor(
        &g_sContext,
        GRAPHICS_COLOR_HOT_PINK); // make horizontal line blue
    GrContextFontSet(&g_sContext, &g_sFontlucidabright6x12);
    Graphics_drawStringCentered(&g_sContext, "ctr", AUTO_STRING_LENGTH, 65, 102,
                                OPAQUE_TEXT); // Display number
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    // make horizontal line blue
    GrContextFontSet(&g_sContext, &g_sFontfixed7x13);
    Graphics_drawStringCentered(&g_sContext, "Font", AUTO_STRING_LENGTH, 90, 50,
                                OPAQUE_TEXT); // Display number
    char str[3];
    int i = 0;
    while ((P1IN & S1) != 0)
    {
      Graphics_setForegroundColor(&g_sContext,
                                  GRAPHICS_COLOR_PURPLE); // make counter purple
      sprintf(str, "%d", i);   // Convert number to string
      __delay_cycles(2000000); // wait a second
      Graphics_drawStringCentered(&g_sContext, str, AUTO_STRING_LENGTH, 37, 100,
                                  OPAQUE_TEXT); // Display number
      if (i == 256)
      {
        // reset counter by making last number same color as background before
        // repeating
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawStringCentered(&g_sContext, str, AUTO_STRING_LENGTH, 37,
                                    100, OPAQUE_TEXT);
        i = 0;
      }
      i += 1;
    }

    __delay_cycles(8000000);
  }
}
