#include <msp430fr6989.h>
#include "aclk_config.h"

#define red BIT0     // Port 1.0
#define green BIT7   // Port 9.7

#define but1 BIT1    // Port 1.1
#define but2 BIT2    // Port 1.2

typedef enum {
    RedFast, RedNormal, RedSlow, RedGreen, GreenSlow, GreenNormal, GreenFast
} led_state_t;

// Volatile and global for access by both ISRs
volatile led_state_t state = RedGreen;

void crawler(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
    PM5CTL0 &= ~LOCKLPM5;     // Enable the GPIO pins

    // Configure LEDs
		// Output Directions of LEDs
    P1DIR |= red;
    P9DIR |= green;
		// Turning off the LEDs
    P1OUT &= ~red;
    P9OUT &= ~green;

    // Configure buttons
    P1DIR &= ~(but1 | but2);    // Setting buttons as inputs
    P1REN |= (but1 | but2);     // Enable resistors for buttons
    P1OUT |= (but1 | but2);     // Set resistors as pull-up
    P1IES |= (but1 | but2);     // Set interrupt on falling edge
		P1IE  |= (but1 | but2);     // Enable interrupts proper
		P1IFG &= ~(but1 | but2);    // Set the interrupt flag off as sanity check

    // Configure 32Khz clock
    config_ACLK_to_32KHz_crystal();

    // ACLK | Divide by 1 | Up Mode | Clear Timer
    TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
    // Enable Channel 0 interrupt / disable flag
    TA0CCTL0 |= CCIE;
    TA0CCTL0 &= ~CCIFG;
    // Set time to default 1 second for RedGReen
    TA0CCR0 = 32768 - 1;

    // Lowest power mode which enables ACLK
    _low_power_mode_3();

    for (;;) 
		{
			// Infinite Loop
		}
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void TimerISR()
{
    switch(state)
    {
			case RedFast:
			case RedNormal:
			case RedSlow:
					// Toggle the red LED
					P1OUT ^= red;
					break;
			case RedGreen:
					// Toggle both LEDs
					P1OUT ^= red;
					P9OUT ^= green;
					break;
			case GreenSlow:
			case GreenNormal:
			case GreenFast:
					// Toggle the green LED
					P9OUT ^= green;
					break;
    }
}

#pragma vector = PORT1_VECTOR
__interrupt void ButtonISR()
{
    // 5 ms debounce delay
    _delay_cycles(5000);
    // Check for both the interrupt flag and button
    // press to prevent bouncing
    int but1press = (P1IFG & but1) != 0 && (P1IN & but1) == 0;
    int but2press = (P1IFG & but2) != 0 && (P1IN & but2) == 0;

    if (but1press || but2press)
    {
        // Turn off LEDs for state change
        P1OUT &= ~red;
        P9OUT &= ~green;
        // Move state to the next left state
        if (but1press && state != RedFast)
				{
            state--;
        }
        else if (but2press && state != GreenFast)
				{
            state++;
        }
        // Set time based on updated state
        switch(state)
        {
					case RedFast:
					case GreenFast:
							TA0CCR0 = (32768 / 8) - 1; // 1/8 s
							break;
					case RedNormal:
					case GreenNormal:
							TA0CCR0 = (32768 / 4) - 1; // 1/4 s
							break;
					case RedSlow:
					case GreenSlow:
							TA0CCR0 = (32768 / 2) - 1; // 1/2 s
							break;
					default:
							TA0CCR0 = (32768) - 1; // 1s, default for RedGreen
							break;
        }
        // Reset timer
        TA0R = 0;
        TA0CTL &= ~TAIFG;
    }
		// clear interrupt flags
    P1IFG &= ~(but1 | but2);
}
