/*
 * lcd_driver.h
 *
 *  Created on: Feb 10, 2020
 *      Author: cbraun3
 */

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#define CLEAR 0x01
#define RETURN_HOME 0x02
#define SET_DDRAM_ZERO 0x80
#define ENTRY_MODE_SHIFT_RIGHT 0x06 // Consider changing to 0x06 (to not do with display)
#define CURSORS_DISPLAY_BLINK_ON 0x0F

void delay_ms(unsigned int x);
void sendInstruction(unsigned char instruction);
void sendData(unsigned char instruction);
void lcd_sendString(char* text);
void lcd_setCursor(unsigned char position);
void lcd_clear();

struct LCDController {
	unsigned char rs; // register select bit. 0 = instruction, 1 = data. Controlled by P2.0
	unsigned char e; // enable register bit. 0 = disabled, 1 = enabled. Controlled by P2.1
	unsigned char data; // data register bits DB7-DB4. Stored in the lower 4 bits of this variable. Controlled by P2.2-P2.5 -> DB4-DB7
};

typedef struct LCDController LCDController;

LCDController lcd;

/*
 * Update the pins (P2.2-P2.5) based on the current values in the LCDController.
 * This has been tested to work. Note that P2.2 is DB4, P2.3 is DB5, etc.
 * Considering if we can rework this so we can write to the controller and directly impact the pins
 */
void updatePins() {
	P2OUT = lcd.rs ? P2OUT | 1 << 0 : P2OUT & ~(1 << 0); // Assign rs to P2.0
	P2OUT = lcd.e ? P2OUT | 1 << 1 : P2OUT & ~(1 << 1); // Assign e to P2.1
	unsigned char i;
	// iterate over the individual bits of data and set P2.i high or low based on that
	for (i = 0; i < 4; i++) {
		unsigned char bit = lcd.data & 1 << i; // Select the ith bit of data (starting from the right)
		P2OUT = bit ?
				P2OUT | /*This could be 1 << i+2*/bit << 2 :
				P2OUT & ~(1 << i + 2); // Assign that bit to the correct pin
	}
}

void init_lcd() {
	P2DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
	lcd.rs = 0; // Set to instruction register
	lcd.e = 0; // Don't enable yet
	lcd.data = 0x03; // Prepare for setting into 4-bit mode
	updatePins(); // Update the output pins
	delay_ms(20); // Wait 20 ms (need to wait at least 15ms)

	// Block for first instruction 0x03
	lcd.e = 1;
	updatePins(); // Instruction passed: 0x03
	delay_ms(20); // Wait 20 ms (need to wait at least 4.1ms)

	// block for second instruction 0x03
	lcd.e = 0;
	updatePins();
	delay_ms(5); // Probably don't need this. Test without later
	lcd.e = 1;
	updatePins(); // Instruction passed: 0x03
	delay_ms(10); // Wait 10 ms (need to wait at least 100us)

	// block for third instruction 0x03
	lcd.e = 0;
	updatePins();
	delay_ms(5); // Probably don't need this. Test without later
	lcd.e = 1;
	updatePins(); // Instruction passed: 0x03
	delay_ms(5); // Wait 5 ms (need to wait at least 100 us)

	// block for 4th instruction (enter 4-bit mode) 0x2C
	lcd.e = 0;
	lcd.data = 0x02; // Set the data to change to 4-bit mode. Might need to do in separate step from e
	updatePins();
	delay_ms(5);
	lcd.e = 1;
	updatePins(); // Instruction passed: 0x02
	delay_ms(5); // Probably don't need this. Test without later
	lcd.e = 0;
	updatePins();
	delay_ms(5); // Probably don't need this. Test without later
	lcd.e = 1;
	updatePins(); // Half-Instruction passed: 0x02;
	delay_ms(5); // Probably don't need this. Test without later
	lcd.e = 0;
	lcd.data = 0xC; // Store b(1100) -> 2 line, 5x10 dots
	updatePins();
	delay_ms(5); // Probably don't need this. Test without later
	lcd.e = 1;
	updatePins(); // Second Half-Instruction passed: 0x0C. Total: 0x2C = 00101100
	delay_ms(5); // Probably don't need this. Test without later

	sendInstruction(RETURN_HOME); // Set cursor to home
	sendInstruction(ENTRY_MODE_SHIFT_RIGHT); // Entry mode set to increment cursor after operations
	sendInstruction(CURSORS_DISPLAY_BLINK_ON); // Does same as above commented out block (display on, blinking cursor on)
	sendInstruction(CLEAR);
	sendInstruction(SET_DDRAM_ZERO);

	lcd_sendString("Long message of text on the screen");
	//lcd_clear();
	lcd_setCursor(17);
	//sendInstruction(SET_DDRAM_ZERO | 0x40);

	while (1)
		;

}

/*
 * Send an instruction to the LCD.
 * Intended for internal use.
 */
void sendInstruction(unsigned char instruction) {
	lcd.rs = 0; // Set into instruction mode
	lcd.e = 0; // Start enable low
	lcd.data = (instruction & 0xF0) >> 4; // Set upper bit of data
	updatePins();
	delay_ms(1);
	lcd.e = 1;
	updatePins();
	delay_ms(1);
	lcd.e = 0;
	lcd.data = instruction & 0x0F;
	updatePins();
	delay_ms(1);
	lcd.e = 1;
	updatePins();
	delay_ms(1);
	lcd.e = 0;
	updatePins();
}

/*
 * Send data to the LCD.
 * Intended for internal use
 */
void sendData(unsigned char data) {
	lcd.rs = 1; // Set into instruction mode
	lcd.e = 0; // Start enable low
	lcd.data = (data & 0xF0) >> 4; // Set upper bit of data
	updatePins();
	delay_ms(1);
	lcd.e = 1;
	updatePins();
	delay_ms(1);
	lcd.e = 0;
	lcd.data = data & 0x0F;
	updatePins();
	delay_ms(1);
	lcd.e = 1;
	updatePins();
	delay_ms(1);
	lcd.e = 0;
	updatePins();
}

/*
 * Write a string to the LCD, starting from the first line at 0. Text will go onto the second line.
 * A string longer than 32 characters will be truncated
 */
void lcd_sendString(char* text) {
	unsigned int i = 0;
	for (; *text != 0; text++) {
		lcd_setCursor(i);
		sendData(*text);
		i++;
		if(i == 32)
			return;
	}
}

/*
 * Clear the LCD, setting the cursor back to 0
 */
void lcd_clear() {
	sendInstruction(CLEAR);
}

/*
 * Set the cursor position. 0 is the first character on the first line, 16 is first character on the second line
 */
void lcd_setCursor(unsigned char position) {
	position < 15 ?
			sendInstruction(SET_DDRAM_ZERO | position) :
			sendInstruction(SET_DDRAM_ZERO | (48 + position));
}
//***********************************************************************
//* delay_ms
//*
//* Phil Walter
//*
//* Delay by x milliseconds
//***********************************************************************
void delay_ms(unsigned int x) {
	TA2CCR0 = 1000;		// Set timer TA2 for 1 ms using SMCLK
	TA2CTL = 0x0214;	// Start timer TA2 from zero in UP mode using SMCLK
	while (1)			// Loop
	{
		if (TA2CTL & BIT0)			// Is TA2 flag set?
		{				// if yes do
			TA2CTL = TA2CTL & ~BIT0;				// Reset TA2 flag (TAIFG)
			x--;					// Decrement interval counter (# ms)
			if (x == 0)
				break;	// if counter = 0 break out of loop
		}	//end if(flag)			// we have reached x milliseconds
	}					//end while(1)
	TA2CTL = 0x0204;	// Stop timer TA2
}						//end delay_ms(x)

#endif /* LCD_DRIVER_H_ */
