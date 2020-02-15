#include <msp430.h> 
#include "lcd_driver.h"


#define ENABLE_PINS 0xFFFE // Required to use inputs and outputs
/*
 * main.c
 */

// See page 42 for setting up the LCD



void main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    PM5CTL0 = ENABLE_PINS; // Required to use inputs and outputs

    //P1DIR |= BIT0;
    //P1OUT &= ~BIT0;

    init_lcd();
}
