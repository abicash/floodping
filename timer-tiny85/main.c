#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <string.h>

#include        "usiTwiSlave.h"

// Note: The LSB is the I2C r/w flag and must not be used for addressing!
#define         SLAVE_ADDR_ATTINY       0b10101010

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

volatile long lWDTcounter = 0;
long lWDTMaxOff = 60000 >> 4;
long lWDTMaxOn = 10000 >> 4;

ISR(WDT_vect) {
	cli();
	WDTCR |= (1 << WDIE) | (1 << WDE);
	lWDTcounter++;
	sei();
}

bool DoI2c() {
	cli();
	if ((rxbuffer[3] == 0x99)) {
		return (true);
	} else if ((rxbuffer[3] == 0xaa) && (rxbuffer[8] == 0xaa)) {
		rxbuffer[3] = 0;
		rxbuffer[8] = 0;

		lWDTMaxOff = (rxbuffer[4] | (rxbuffer[5] << 8) | (rxbuffer[6] << 16)
				| (rxbuffer[7] << 24)) >> 4;
	} else if ((rxbuffer[3] == 0xbb) && (rxbuffer[8] == 0xbb)) {
		rxbuffer[3] = 0;
		rxbuffer[8] = 0;

		lWDTMaxOn = (rxbuffer[4] | (rxbuffer[5] << 8) | (rxbuffer[6] << 16)
				| (rxbuffer[7] << 24)) >> 4;
	}
	sei();
	return (false);
}

void SleepLoop(long lMax) {

	lWDTcounter = 0;

	while (1) {

		wdt_reset();
		if (lWDTcounter >= lMax) {
			break;
		}
		DoI2c();

		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_cpu();

		sleep_disable();
		wdt_reset();
	}
}

int main(void) {
	cli();
	wdt_reset();
	WDTCR |= (1 << WDIE) | (1 << WDE);

	sbi(DDRB, PB4);
	sbi(DDRB, PB3);

	if (bis(MCUSR, WDRF)) {
		cbi(MCUSR, WDRF);
	}

	usiTwiSlaveInit(SLAVE_ADDR_ATTINY);

	sei();

	while (1) {

		sbi(PORTB, PB4);
		cbi(PORTB, PB3);
		SleepLoop(lWDTMaxOn);

		cbi(PORTB, PB4);
		sbi(PORTB, PB3);
		SleepLoop(lWDTMaxOff);

	}
}
