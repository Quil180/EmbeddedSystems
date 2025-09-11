// Code that flashes the red LED
#include <msp430fr6989.h>
#include <stdint.h> // for uint32_t

#define redLED BIT0 // P1.0
#define greenLED BIT7 // P9.7

void main(void)
{
    volatile uint32_t i; // solution 2
    WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
    PM5CTL0 &= ~LOCKLPM5; // Disables GPIO power on by default to high impedance

    P1DIR |= redLED;
    P1OUT &= ~redLED;

    P9DIR |= greenLED; // Direct pin as output
    P9OUT &= ~greenLED; // Turns led off

//    for(;;)
//    {
////        P9OUT ^= greenLED; // This makes it out of sync
//        // Solution 2
//        for(i=0; i<200000; i++) // Generates a delay of ~3.12 seconds
//        {
//            // Nothing, for delay
//        }
//        // solution 3
////        _delay_cycles(3000000); // 3*10^6 cycle stop or 3 seconds worth
//        P1OUT ^= redLED; // Toggle the led via xor
//        P9OUT ^= greenLED; // In-sync
//    }

    // Fire truck pattern 1
    for (;;)
    {
        int j;
        int k;
        for (k = 0; k < 2; k++)
        {
            for (j = 0; j < 8; j++)
            {
                P1OUT ^= redLED;
                _delay_cycles(31250);
            }
            for (j = 0; j < 8; j++)
            {
                P9OUT ^= greenLED;
                _delay_cycles(31250);
            }
        }
        for (k = 0; k < 2; k++)
        {
            // half
            for (j = 0; j < 8; j++)
            {
                P1OUT ^= redLED;
                _delay_cycles(62500);
            }
            for (j = 0; j < 8; j++)
            {
                P9OUT ^= greenLED;
                _delay_cycles(62500);
            }
        }
    }

    // Fire Truck Pattern 2
//    for (;;)
//    {
//        int j;
//        for (j = 0; j < 8; j++)
//        {
//            P9OUT ^= greenLED; // This makes it out of sync
//            _delay_cycles(500000); // 3*10^6 cycle stop or 3 seconds worth
//            P1OUT ^= redLED; // Toggle the led via xor
//        }
//        for (j = 0; j < 8; j++)
//        {
//            P9OUT ^= greenLED; // This makes it out of sync
//            _delay_cycles(250000); // 3*10^6 cycle stop or 3 seconds worth
//            P1OUT ^= redLED; // Toggle the led via xor
//        }
//        for (j = 0; j < 8; j++)
//        {
//            P9OUT ^= greenLED; // This makes it out of sync
//            _delay_cycles(125000); // 3*10^6 cycle stop or 3 seconds worth
//            P1OUT ^= redLED; // Toggle the led via xor
//        }
//
//        P9OUT &= ~greenLED; // Turns green off
//        P1OUT &= ~redLED; // Turns red off
//
//        // On then off 3 times
//        for (j = 0; j < 8; j++)
//        {
//            P9OUT ^= greenLED;
//            P1OUT ^= redLED;
//            _delay_cycles(250000);
//        }
//    }
}
