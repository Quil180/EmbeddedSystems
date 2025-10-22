// Lab 06 Main

#include <msp430.h>
#include <msp430fr6989.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


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
const unsigned char LCD_Shapes[10] = {0xFC,0x60,0xDB,0xF3,
0x67,0xB7,0xBF,0xE0, 0xFF, 0xF7} ;

volatile int number = 0;

volatile int runway1State = 0;
volatile int runway2State = 0;
volatile unsigned int LuxValue = 0;

volatile int time = 0;
volatile int current_time = 50000;


char HH_c[] = "00";
char MM_c[] = "00";
char SS_c[] = "00";
//volatile int LuxValue;
volatile int prevBase;

void uart_write_uint16 (unsigned int n);
void uart_write_string(char * str);





void uart_write_char(unsigned char ch)
{
    // Wait for any ongoing transmission to complete
    while ( (FLAGS & TXFLAG)==0 ) {}
    // Copy the byte to the transmit buffer
    TXBUFFER = ch; // Tx flag goes to 0 and Tx begins!
    return;
}

// The function returns the byte; if none received, returns null character
unsigned char uart_read_char(void)
{
    unsigned char temp;
    // Return null character (ASCII=0) if no byte was received
    if( (FLAGS & RXFLAG) == 0)
        return 0;
    // Otherwise, copy the received byte (this clears the flag) and return it
    temp = RXBUFFER;
    return temp;
}


void uart_write_uint16(unsigned int n)
{
    if(n == 0)
    {
        uart_write_char(48);
    }
    else 
    {
        unsigned int digits = (int)log10(n) + 1;
        while(digits >0)
        {
            digits = digits -1;
            unsigned int current_digit = n / (int)pow(10, digits);
            n = n - current_digit * (int)pow(10, digits);
            uart_write_char(48 + current_digit);
        }
    }
    //uart_write_char('\n');
    //uart_write_char('\r');

    return;
}

void uart_write_string(char * str)
{
    unsigned int string_length = strlen(str);
    unsigned int i = 0;
    for(i = 0; i < string_length; i++)
    {
        uart_write_char(str[i]);
    }
    return;
}

// Configure UART to the popular configuration
// 9600 baud, 8-bit data, LSB first, no parity bits, 1 stop bit
// no flow control, oversampling reception
// Clock: SMCLK @ 1 MHz (1,000,000 Hz)
void Initialize_UART(void)
{
    // Configure pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);
    // Main configuration register
    UCA1CTLW0 = UCSWRST; // Engage reset; change all the fields to zero
    // Most fields in this register, when set to zero, correspond to the
    // popular configuration
    UCA1CTLW0 |= UCSSEL_2; // Set clock to SMCLK
    // Configure the clock dividers and modulators (and enable oversampling)
    UCA1BRW = 6; // divider
    // Modulators: UCBRF = 8 = 1000 --> UCBRF3 (bit #3)
    // UCBRS = 0x20 = 0010 0000 = UCBRS5 (bit #5)
    UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
    // Exit the reset state
    UCA1CTLW0 &= ~UCSWRST;
}



//**********************************
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




void Initialize_I2C(void) 
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

//Read two Bytes from the an internal register on the I2C device
// Data is passed by reference (aka pointers)
//int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data);


// Writes two Bytes to the a register on the I2C
// i2c_write_word(0x22, 0x60, data)
// data = 0xABCD
// 0x60 -> I2C register
//int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data);


////////////////////////////////////////////////////////////////////
// Read a word (2 bytes) from I2C (address, register)
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data) 
{
    unsigned char byte1=0, byte2=0; // Intialize to ensure successful reading
    UCB1I2CSA = i2c_address; // Set address
    UCB1IFG &= ~UCTXIFG0;
    // Transmit a byte (the internal register address)
    UCB1CTLW0 |= UCTR;
    UCB1CTLW0 |= UCTXSTT;
    while((UCB1IFG & UCTXIFG0)==0) {} // Wait for flag to raise
    UCB1TXBUF = i2c_reg; // Write in the TX buffer
    while((UCB1IFG & UCTXIFG0)==0) {} // Buffer copied to shift register;
    //Tx in progress; set Stop bit
    // Repeated Start
    UCB1CTLW0 &= ~UCTR;
    UCB1CTLW0 |= UCTXSTT;
    // Read the first byte
    while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
    byte1 = UCB1RXBUF;
    // Assert the Stop signal bit before receiving the last byte
    UCB1CTLW0 |= UCTXSTP;
    // Read the second byte
    while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
    byte2 = UCB1RXBUF;
    while((UCB1CTLW0 & UCTXSTP)!=0) {}
    while((UCB1STATW & UCBBUSY)!=0) {}

    *data = (byte1 << 8) | (byte2 & (unsigned int)0x00FF);

    return 0;
}

////////////////////////////////////////////////////////////////////
// Write a word (2 bytes) to I2C (address, register)
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data) 
{
        unsigned char byte1, byte2;

        UCB1I2CSA = i2c_address; // Set I2C address

        byte1 = (data >> 8) & 0xFF; // MSByte
        byte2 = data & 0xFF; // LSByte

        UCB1IFG &= ~UCTXIFG0;

        // Write 3 bytes
        UCB1CTLW0 |= (UCTR | UCTXSTT);
        while( (UCB1IFG & UCTXIFG0) == 0) {}
        UCB1TXBUF = i2c_reg;
        while( (UCB1IFG & UCTXIFG0) == 0) {}
        UCB1TXBUF = byte1;
        while( (UCB1IFG & UCTXIFG0) == 0) {}
        UCB1TXBUF = byte2;
        while( (UCB1IFG & UCTXIFG0) == 0) {}
        UCB1CTLW0 |= UCTXSTP;
        while( (UCB1CTLW0 & UCTXSTP) != 0 ) {}
        while((UCB1STATW & UCBBUSY)!=0) {}
    return 0;
}


void update_clock_numbers(unsigned int n)
{
    // A1 & A2 hours
    // A3 & A4 Mins
    // A5 & A6 seconds
    // assume numbers
    //divide by # secs in hours
    // divide by # secs in hours
    // divide by # 

    volatile int HH = 0;
    volatile int MM = 0;
    volatile int SS = 0;
    

    volatile int current_digit = 0;
    current_digit = n %3600;    // gets current # hours


    // later edge case uh 
    //Seconds Logic 
    unsigned int seconds = n % 60;
    if(seconds >0)
    {
        SS = seconds % 10;
        if(seconds >10)
        {
            SS += (seconds / 10) *10;
        }
        
    }
    //uart_write_string("Seconds here:\n");
    //uart_write_uint16(SS);
    //uart_write_string()
    n = n/60;
    //Minutes Logic 
    unsigned int minutes = n % 60;
    if(minutes >0)
    {
        // add the one digits to the thing
        MM += minutes % 10;

        if(minutes >10)
        {
            MM += (minutes / 10) * 10;
        }
        //LCDM19 = LCD_Shapes[minutes % 10];
    }
    n = n/60;
    //Hours Logic
    unsigned int hours = n % (60);
    if(hours >0)
    {
        if(hours >10)
        {
            HH += (hours / 10) * 10;
        }
        HH += hours % 10;
    }
    uart_write_uint16(HH);
    uart_write_char(':');
    uart_write_uint16(MM);
    uart_write_char(':');
    uart_write_uint16(SS);
    uart_write_char('\t');
}



void init_lux_logger()
{

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

    P1IES |=  (BUT1 | BUT2);      //1: Interrupt on falling edge (0for rising edge)
    P1IFG &= ~(BUT1 | BUT2);    //0: Clear the interrupt flags
    P1IE  |=  (BUT1 | BUT2);      //1: Enable the interrupts
    
    config_ACLK_to_32KHz_crystal();
    //Initialize_UART();  // swap back to smlclk  

    // standard delay is 1 second with interrupts
    // Configure channel 0 for up mode with interrupts
    TA0CCR0 = 32768;            // 1 second @ 32kHz 
    TA0CCTL0 |= CCIE;           // Enable channel 0 CCIE1
    TA0CCTL0 &= ~CCIFG;         // Clear Channel 0 CCIFG

    // Use ACLK, divide by 1, up mode, clear TAR 
    TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;

    // Ensure flag is cleared at the start
    TA0CTL &= ~TAIFG;
   
   // Enable Global Interrupt bit ( call an intrinsic function)
   _enable_interrupt();
   Initialize_I2C();
   Initialize_UART();

    //writePretty();

    // uart_write_string("Hello Test\r\n");
    // uart_write_uint16(65535);
    // uart_write_uint16(6652);
    // uart_write_uint16(1);
    //uart_write_uint16(0);

    i2c_write_word(0x44,0x01, 0x7604);
    

    P1OUT |= redLED;
    uart_write_string("done with init\n");
    _delay_cycles(600000);
    i2c_read_word(0x44, 0x00, &LuxValue);
    prevBase = LuxValue;

    //uart_write_uint16(LuxValue);

   //unsigned int count = 0;
    for(;;)
    {   
        //if(TA0)
        _delay_cycles(50000);
        P1OUT ^= redLED;                // confirm loop in action

        
    }    
}

// one second Timer system
// interrupt for blinking
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TA00_ISR()
{
    P9OUT ^= greenLED;

    // // Part 1
    // unsigned int data1, data2;

    // // // grabs the data via reference and gets them read
    // i2c_read_word(0x44, 0x7E,  &data1);
    // i2c_read_word(0x44, 0x7F,  &data2);
    

    // uart_write_uint16(data1);
    // uart_write_uint16(data2);

    

    if(time == 1)
    {
        i2c_read_word(0x44, 0x00, &LuxValue);
        //uart_write_uint16(current_time);
        //update_clock_numbers(current_time);
        update_time();
        uart_write_char('\t');
        uart_write_uint16(LuxValue);
        uart_write_string(" lux");

        if(LuxValue > prevBase + 10)
        {
            uart_write_string("\t<Up>\n");
            prevBase = LuxValue;
        }
        else if(LuxValue < prevBase - 10)
        {
            uart_write_string("\t<Down>\n");
            prevBase = LuxValue;
        }
        else
        {
            uart_write_char('\n');
        }
        time =0;
    }
    time = time + 1;
    current_time = current_time+1;
    // Part 2
    
        
}

void update_time()
{
    // if it is 9
    if(MM_c[1] == 57)
    {
        
        if(MM_c[0] < 53)
        {
            MM_c[0]++;
            MM_c[1] = 48;
        }
        else {
            //now increase hour by one
            MM_c[0] = 48;
            MM_c[1] = 48;
            // check last possible time, else go up
            if(HH_c[0] == 50 && HH_c[1]==51)
            {
                //MM_c[0] = 48;
                //MM_c[1] = 48;
                HH_c[0] = 48;
                HH_c[1] = 48;
            }else if(HH_c[1] == 57)
            {
                HH_c[0]++;
                HH_c[1] = 48;
            }
            else {
                HH_c[1]++;
            }
        }
    }
    else {
        MM_c[1]++;
    }
    print_time();
    
}

// S2 adjust time

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
            if(given_char ==0)
            {
                continue;
            }
            
            //uart_write_char('\n');
            //uart_write_uint16(given_char);
            //uart_write_char('\n');
            //uart_write_char(given_char);
            if(given_char == 3 || given_char == 27 || given_char == 13)
            {
                break;
            }
            time[loc] = given_char;
            loc++;
        } 
        uart_write_string("got out!\n");

        if(time[3] == 95)
        {
            uart_write_string("entered 3");
            HH_c[0] = 48;
            HH_c[1] = time[0];
            MM_c[0] = time[1];
            MM_c[1] = time[2];
        }
        else {
            uart_write_string("entered 4");
            HH_c[0] = time[0];
            HH_c[1] = time[1];
            MM_c[0] = time[2];
            MM_c[1] = time[3];
        
        }

        uart_write_string("Time set to ");
        print_time();
        uart_write_char('\n');
        //update_clock_numbers(time);
        // unsigned char char1 = uart_read_char();
        // unsigned char char2 = uart_read_char(); 
        // unsigned char char3 = uart_read_char(); 
        
        // uart_write_char(char1);
        // uart_write_char(char2);
        // uart_write_char(char3);
        // if(char3 = '\n')
        // {
        //     uart_write_string("entered 3 numbers");
        // }
        // else {
        //     char char4 =uart_read_char();


        // }
        // uart_write_string("read interrupt\n");
        P1IFG &= ~BUT2;
        TA0CTL |= MC_1;
    }
    
}
















// // Part 4 approach
// // interrupt for button reading
// #pragma vector = PORT1_VECTOR // Write the vector name
// __interrupt void Port1_ISR() {

//     // button bounce blocker
//     _delay_cycles(5000);
    
//     // Detect button 1 interrupt flag
//     if (P1IFG & BUT1) {
//         // Button 1 action
        
//         P1IFG &= ~BUT1;   // clear the flag
//         if(runway1State == 1){ // if requested
//             runway1State = 2; //move to in use
//             P1OUT &= ~redLED;
//         }else if (runway1State == 2) {
//             runway1State = 3;
//         }
//     }
//     // Detect button 2 interrupt flag
//     if (P1IFG & BUT2) {
//         // Button 2 action
        
//         P1IFG &= ~BUT2;   // clear the flag
//         if(runway2State == 1){      // if requested
//             runway2State = 2;    //move to in use
//             P9OUT &= ~greenLED; // Turn LED Off
//         }else if (runway2State == 2) {
//             runway2State =3;   
//         }
//     }
//     updateScreen();
//     // Reset them
    
    

//     // //adjust speed depending on value
//     // if(state == 0){
//     //     TA0CCR0 = 32768;
//     // }else {
//     //     TA0CCR0 = 32768 / abs(state * 2 ) -1;
//     // }
// }




// void towerCom()
// {
//     unsigned char read_char = uart_read_char();
//     if (read_char == '3')
//     {
//         if(runway2State == 0){      // if requested
//             runway2State = 1;
//             P9OUT |= greenLED;
//         }
//         else if (runway2State == 2) {
//             runway2State =3;   
//         }
//     }
//     else if (read_char == '1')
//     {
//         if(runway1State == 0){      // if requested
//             runway1State = 1;
//             P1OUT |= redLED;
//         }   
        
//     }
//     else if (read_char == '9')
//     {
//         if(runway2State >= 2){      // if requested
//             runway2State = 0;
//             P9OUT &= ~greenLED;
//             if(runway2State ==0)
//             {
//                 uart_write_string("\033[11;33H            ");
//                 uart_write_string("\033[12;33H            ");
//                 uart_write_string("\033[14;33H                   ");
//                 uart_write_string("\033[15;0H");
//             }
//         }
//     }
//     else if (read_char == '7')
//     {
//         if(runway1State >= 2){      // if requested
//             runway1State = 0;
//             P1OUT &= ~redLED;
//             if(runway1State ==0)
//             {
//                 uart_write_string("\033[11;0H            ");
//                 uart_write_string("\033[12;0H            ");
//                 uart_write_string("\033[14;0H                   ");
//                 uart_write_string("\033[15;0H");
//             }
//         }   
        
//     }
//     else if (read_char == 'r')
//     {
//         clearUart();        
//     }
//     if(read_char)
//     {
//         updateScreen();
//     }
// }

// void writePretty()
// {
//     clearUart();
//     //uart_write_string("\033[12A");
//     //uart_write_string("\0331;1H");

//     uart_write_string("ORLANDO EXECUTIVE AIRPORT RUNWAY CONTROL\r\n");
//     uart_write_string("\r\n\r\n");


//     uart_write_string("                Runway 1    Runway 2\r\n");
//     uart_write_string("Request (RQ):       1           3\r\n");
//     uart_write_string("Forfeit (FF):       7           9\r\n\r\n");


//     uart_write_string("--------                        -------\r\n");
//     uart_write_string("RUNWAY 1                        RUNWAY2\r\n");
//     uart_write_string("--------                        -------\r\n\r\n");
//     uart_write_string("\033[15;0H");
// }

// void updateScreen()
// {

//     // formatting hell for state update
//     //uart_write_string("\033[10;0H");
//     if(runway1State >=1)
//     {
//     uart_write_string("\033[11;0HRequested\r\n");
//     }
//     if(runway1State >=2)
//     {
//     uart_write_string("\033[12;0HIn use\r\n");
//     }
//     if(runway1State >=3)
//     {
//     uart_write_string("\r\n\033[14;0H*** Inquiry ***\r\n");
//     }



//     if(runway2State >=1)
//     {
//     uart_write_string("\033[11;33HRequested\r\n");
//     }
//     if(runway2State >=2)
//     {
//     uart_write_string("\033[12;33HIn use\r\n");
//     }
//     if(runway2State >=3)
//     {
//     uart_write_string("\r\n\033[14;33H*** Inquiry ***\r\n");
//     }

//     uart_write_string("\033[15;0H");
//     //uart_write_string("R1 State");
//     // uart_write_string("");
//     // uart_write_uint16(runway1State);

//     // uart_write_string("R2 State");
//     // uart_write_uint16(runway2State);

//     // uart_write_string("\r\n");
//     //uart_write_string("\033[3A");
//     return;



//     // --------                        -------
//     // RUNWAY 1                        RUNWAY2
//     // --------                        -------
//     // Requested                       Requested
//     // In use                          In use

//     // *** Inquiry ***                 *** Inquiry ***



// }


void clearUart()
{
    // reset top
    uart_write_string("\033[2J");
    uart_write_string("\033[12A");
    //uart_write_string("\0331;1H");
    
}
