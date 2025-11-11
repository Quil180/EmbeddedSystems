#include "msp430fr6989.h"

#define PWM BIT0 // P1.0
#define PWM_VAL 33
#define UP 3500
#define DOWN 600

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

void Initialize_ADC(void) 
{
  // Configure the pins to analog functionality
  // X-axis: A10/P9.2, for A10 (P9DIR=x, P9SEL1=1, P9SEL0=1)
  P9SEL1 |= BIT2;
  P9SEL0 |= BIT2;
  // Turn on the ADC module
  ADC12CTL0 |= ADC12ON;
  // Turn off ENC (Enable Conversion) bit while modifying the configuration
  ADC12CTL0 &= ~ADC12ENC;
  //*************** ADC12CTL0 ***************
  // ADC12SHT0x sets SHT cycles for results 0-7, 24-31
  // ADC12MSC sets multiple analog inputs
  // Sets SHT of 16 cycles (found in doc. slau367o table 34.4)
  ADC12CTL0 |= ADC12SHT0_2; 
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
  // ADC12CTL1 |= ADC12CONSEQ_1;
  // values in doc. slau367o table 34.5
  //*************** ADC12CTL2 ***************
  // ADC12RES sets bit resolution
  // ADC12DF sets data format
  ADC12CTL2 |= ADC12RES_2; // 2 = 12-bit
  ADC12CTL2 &= ~ADC12DF;   // 0 = unsigned binary
  //*************** ADC12MCTL0 ***************
  // ADC12VRSELx sets VR+ and VR- sources as well as buffering
  // ADC12INCHx sets analog input
  ADC12MCTL0 |= ADC12VRSEL_0; // 0 -> VR+ = AVCC and VR- = AVSS
  ADC12MCTL0 |= ADC12INCH_10; // 10 = A10 input
  //*************** ADC12MCTL1 ***************
  // set ENC bit at end of config
  ADC12CTL0 |= ADC12ENC;
}

int main(void)
{
  // Configure WDT & GPIO
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;

  // Configure PWM
  P1DIR  |= PWM;
  P1SEL1 &= ~PWM;
  P1SEL0 |= PWM;

  config_ACLK_to_32KHz_crystal();

  TA0CCR0  = 33;  // 33 cycles for 1000Hz
  TA0CCTL0 = CCIE; // Enabling channel 0 interrupt

  TA0CCR1  = 15; // 50% brightness
  TA0CCTL1 = OUTMOD_7; // Reset/Set Output Mode
  
  //       ACLK       /1     Up     Clear TAR
  TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;

  // Initializing ADC
  Initialize_ADC();

  _low_power_mode_3(); // We only need ACLK

  unsigned int x;
  unsigned int y;

  for (;;)
  {
    ADC12CTL0 |= ADC12SC;

    while (ADC12CTL0 & ADC12BUSY)
    {
      // Wait till not busy
    }

    x = ADC12MEM0;
    y = ADC12MEM1;

    if (y > UP)
    {
      // Brightness is maximum
      TA0CCR1 = 32;
    }
    if (y < DOWN)
    {
      // Brightness is off
      TA0CCR1 = 0;
    }

    if (x > UP)
    {
      if (TA0CCR1 > 0)
      {
        TA0CCR1 -= 1;
      }
    }

    if (x < DOWN)
    {
      if (TA0CCR1 < 32)
      {
        TA0CCR1 += 1;
      }
    }

    __delay_cycles(16000); // 1ms delay
  }
  return 0;
}
