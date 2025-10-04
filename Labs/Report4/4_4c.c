#include <msp430fr6989.h>
#include "aclk_config.h"

#define red BIT0     // Port 1.0
#define green BIT7   // Port 9.7

#define but1 BIT1    // Port 1.1
#define but2 BIT2    // Port 1.2

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
	PM5CTL0 &= ~LOCKLPM5;     // Enable the GPIO pins

	P1DIR |= red;     // Set output for red led
	P1OUT &= ~red;    // Turn off red LED

	P9DIR |= green;   // Set output for green led
	P9OUT &= ~green;  // Turn off green LED

	// Configure buttons
	P1DIR &= ~(but1 | but2);    // Setting buttons as inputs
	P1REN |= (but1 | but2);     // Enable resistors for buttons
	P1OUT |= (but1 | but2);     // Set resistors as pull-up
	P1IES |= (but1 | but2);     // Set interrupt on falling edge
	P1IE  |= (but1 | but2);     // Enable interrupts proper
	P1IFG &= ~(but1 | but2);    // Set the interrupt flag off as sanity check

	// Lowest power mode that enables ACLK
	_low_power_mode_3();

	for (;;) 
	{
		// Infinite Loop
	}
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR()
{
	// button1 interrupt raised
	if (!(P1IFG & but1))
	{
		P1OUT ^= red;   // toggle red led
		P1IFG &= ~but1; // turn off flag for button1
	}
	// button2 interrupt raised
	if (!(P1IFG & but2))
	{
		P9OUT ^= green; // toggle green led
		P1IFG &= ~but2; // turn off flag for button 2
	}
}
