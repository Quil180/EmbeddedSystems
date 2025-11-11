void HAL_LCD_PortInit(void)
{
  // Configuring the SPI pins
  // Configure UCB0CLK/P1.4 pin to serial clock
  P1SEL0 |= BIT4;  // on according to data sheet for UCB0CLK
  P1SEL1 &= ~BIT4; // off according to data sheet for UCB0CLK
  // Configure UCB0SIMO/P1.6 pin to SIMO
  P1SEL0 |= BIT6;
  P1SEL1 &= ~BIT6;
  // OK to ignore UCB0STE/P1.5 since we are connecting the display's enable bit
  // to low (enabled all the time).
  // OK to ignore UCB0SOMI/P1.7 since the display doesn't give back any data

  // Configuring the display's other pins
  // Set reset pin as output
  P9DIR |= BIT4;
  // Set the data/command pin as output (P2.3)
  P2DIR |= BIT3;
  // Set the chip select pin as output (P2.5)
  P2DIR |= BIT5;

  return;
}
void HAL_LCD_SpiInit(void)
{
  // SPI configuration

  // Put eUSCI in reset state and set all fields in the register to 0
  UCB0CTLW0 = UCSWRST;

  // Fields that need to be nonzero are changed below
  // Set clock phase to "capture on 1st edge, change on following edge"
  // 1 = latched on the first edge and shifted on the following edge
  UCB0CTLW0 |= UCCKPH; 
  // Set clock polarity to "inactive low" (0)
  UCB0CTLW0 &= ~UCCKPL; 
  // Set data order to "transmit MSB first" (1)
  UCB0CTLW0 |= UCMSB; 
  // Set data size to 8-bit (0)
  UCB0CTLW0 &= ~UC7BIT; 
  // Set MCU to "SPI master" (1)
  UCB0CTLW0 |= UCMST; 
  // Set SPI to "3-pin SPI" (0) (we won't use eUSCI's chip select)
  UCB0CTLW0 |= UCMODE0; 
  // Set module to synchronous mode (1)
  UCB0CTLW0 |= UCSYNC; 
  // Set clock to SMCLK which is 2, in master mode
  UCB0CTLW0 |= UCSSEL_2;
  // Configure the clock divider (SMCLK is set to 16 MHz; run SPI at 8 MHz using
  // SMCLK)
  // Maximum SPI supported frequency on the display is 10 MHz
  UCB0BRW |= 2; // Divider = 2 in order to get 8 Mhz from 16 Mhz clock
  // Exit the reset state (1) at the end of the configuration
  UCB0CTLW0 &= ~UCSWRST; 
  // Set CS' (chip select) bit to 0 (display always enabled)
  P2OUT &= ~BIT5;
  // Set DC' bit to 0 (assume data)
  P2OUT &= ~BIT3;

  return;
}
