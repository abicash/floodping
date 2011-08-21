/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - demo main module to test irmp decoder
 *
 * Copyright (c) 2009-2010 Frank Meyer - frank(at)fli4l.de
 *
 * ATMEGA88 @ 8 MHz
 *
 * Fuses: lfuse: 0xE2 hfuse: 0xDC efuse: 0xF9
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include        "usiTwiSlave.h"

// Note: The LSB is the I2C r/w flag and must not be used for addressing!
#define         SLAVE_ADDR_ATTINY       0b00111100

//####################################################################### Macros

#define uniq(LOW,HEIGHT)        ((HEIGHT << 8)|LOW)                       // Create 16 bit number from two bytes
#define LOW_BYTE(x)             (x & 0xff)                                          // Get low byte from 16 bit number
#define HIGH_BYTE(x)            ((x >> 8) & 0xff)                         // Get high byte from 16 bit number
#define sbi(ADDRESS,BIT)        ((ADDRESS) |= (1<<(BIT)))       // Set bit
#define cbi(ADDRESS,BIT)        ((ADDRESS) &= ~(1<<(BIT)))// Clear bit
#define toggle(ADDRESS,BIT)     ((ADDRESS) ^= (1<<BIT)) // Toggle bit
#define bis(ADDRESS,BIT)        (ADDRESS & (1<<BIT))              // Is bit set?
#define bic(ADDRESS,BIT)        (!(ADDRESS & (1<<BIT)))         // Is bit clear?
//#################################################################### Variables

uint8_t byte1, byte2;
uint16_t buffer;
uint8_t high, low = 0; // Variables used with uniq (high and low byte)

uint16_t AdcRead(uint8_t channel) {
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)) {
	}
	return ADCW;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MAIN: main routine
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
int main(void) {
	cli();

	usiTwiSlaveInit(SLAVE_ADDR_ATTINY); // TWI slave init

	ADMUX |= (1 << REFS1) | (1 << REFS0) | (1 << ADLAR);
	ADCSRA |= (1 << ADEN); // | (1 << ADFR);
	ADCSRA |= (1 << ADSC) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);

	int uiLastDistance = 0;
	int uiMovement = 0;
	int uiLastCmd = 0;
	int uiLastCmdTimer = 0;
	int uiLightCmd = 0;
	int uiTempLight = 0;

	sei ();
	// enable interrupts

	for (;;) {
		for (int uiTmp = 0; uiTmp < 30; uiTmp++) {
			_delay_ms(1);
			wdt_reset();
		}
		int uiNewDistance = AdcRead(0) >> 8;
		int uiMinDiff = 6;

		int uiDiff = uiNewDistance - uiLastDistance;
		if (uiDiff > uiMinDiff) {
			uiMovement++;
		} else if (uiDiff < -uiMinDiff) {
			uiMovement--;
		} else {
			if (uiMovement > 0) {
				uiMovement--;
			} else if (uiMovement < 0) {
				uiMovement++;
			}
		}

		if (uiMovement > 5) {
			switch (uiLastCmd) {
			case 1:
				break;
			case 2:
				uiLightCmd = 1;
				uiTempLight = 5;
				uiLastCmd = 1;
				uiLastCmdTimer = 0;
				break;
			default:
				uiTempLight = 3;
				uiLastCmd = 1;
				uiLastCmdTimer = 0;
				break;
			}
		} else if (uiMovement < -5) {
			switch (uiLastCmd) {
			case 1:
				uiLightCmd = 2;
				uiTempLight = 6;
				uiLastCmd = 2;
				uiLastCmdTimer = 0;
				break;
			case 2:
				break;
			default:
				uiTempLight = 4;
				uiLastCmd = 2;
				uiLastCmdTimer = 0;
				break;
			}
		}

		if (uiLastCmdTimer++ > 50) {
			uiLastCmd = 0;
			uiLastCmdTimer = 50;
			uiTempLight = 0;
		}

		uiLastDistance = uiNewDistance;

		cli();
		if(rxbuffer[1] == 1)
		{
			rxbuffer[1] = 0;
			uiLightCmd = 0;
		}

		txbuffer[0] = uiLightCmd;
		txbuffer[1] = uiTempLight;

		sei();

	}
}
