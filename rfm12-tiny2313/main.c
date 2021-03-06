#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "rf12.h"
#include "main.h"

#include <util/delay.h>

#define AIRID "0005"

void send(char bSend) {
	if (bSend) {
		rf12_init();
		rf12_setfreq(RF12FREQ868(868.3));
		rf12_setbandwidth(4, 1, 4);
		rf12_setbaud(666);
		rf12_setpower(0, 6);
	}

	uchar id[8], diff;
	uchar i = 0;
	uint temp;

	start_meas();


  for (diff = SEARCH_FIRST; diff != LAST_DEVICE && i < 4;) {
		diff = w1_rom_search(diff, id);

		if (diff == PRESENCE_ERR) {
			if (bSend) {
				rf12_txdata("epres", 5);
			}
			break;
		}
		if (diff == DATA_ERR) {
			if (bSend) {
				rf12_txdata("edata", 5);
			}
			break;
		}
		if (id[0] == 0x28 || id[0] == 0x10) { // temperature sensor
			w1_byte_wr(READ); // read command
			temp = w1_byte_rd(); // low byte
			temp |= (uint) w1_byte_rd() << 8; // high byte
			if (bSend) {
				if (id[0] == 0x10) // 9 -> 12 bit
				{
					temp <<= 3;
				}
				char test[32];
				test[0] = 'T';
				memcpy(test + 1, &(temp), 2);
				memcpy(test + 3, id, 8);
				memcpy(test + 11, &(temp), 2);
				memcpy(test + 13, id, 8);
				LED_AN(LED1);
				rf12_txdata(test, 22);
				LED_AUS(LED1);
			}
		}
	}
	if (bSend) {
		rf12_trans(0x8201);
		rf12_trans(0x0);
	}
}

#define MAXCOUNT 30
unsigned int volatile WDTcounter = MAXCOUNT - 1;

ISR(WDT_OVERFLOW_vect)
{
	cli();
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	WDTcounter++;
	sei();
}

int main(void) {
	DDRD |= (1 << LED1) | (1 << LED2); // Port D: Ausgang für LED1 und LED2

	LED_AN(LED1);
	LED_AN(LED2);

	rf12_preinit(AIRID);

	send(0);
	_delay_ms(1000);
	send(0);

	cli();
	wdt_reset();
	wdt_enable (WDTO_1S);
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	sei();

	for (;;) {
		if (WDTcounter >= MAXCOUNT) {
			LED_AN(LED1);
			LED_AN(LED2);
			send(1);
			WDTcounter = 0;
		}
		LED_AUS(LED1);
		LED_AUS(LED2);

		wdt_reset();

		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();

		sleep_cpu ();
		sleep_disable ();
		wdt_reset();
	}
}
