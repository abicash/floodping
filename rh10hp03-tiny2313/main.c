#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "uart.h"
#include "i2cmaster.h"
#include "rf12.h"

#define AIRID "0102"

unsigned int volatile WDTcounter = 0;

ISR(WDT_OVERFLOW_vect) {
	cli();
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	WDTcounter++;
	sei();
}

#define MAXCOUNT 30

static union {
	unsigned char bytes[18];
	struct {
		unsigned short C1, C2, C3, C4, C5, C6, C7;
		unsigned char A, B, C, D;
	} Coeff;
} HP03_Cal;

static struct {
	unsigned short sens, off;
} HH10_Cal;

#define u8 unsigned char
#define u16 unsigned short

#define HP03_ADC_ADDR_W  0xEE
#define HP03_ADC_ADDR_R  0xEF

#define HP03_CAL_ADDR_W  0xA0
#define HP03_CAL_ADDR_R  0xA1

#define HH10_CAL_ADDR_W  0xA2
#define HH10_CAL_ADDR_R  0xA3

int RH[16];
int RHcount = 0;

void HP03_ReadOne(u8 what, u16 *v) {
	i2c_start_wait(HP03_ADC_ADDR_W);
	i2c_write(0xFF);
	i2c_write(what);
	i2c_stop();

	_delay_us(40000);

	i2c_start_wait(HP03_ADC_ADDR_W);
	i2c_write(0xFD);
	i2c_rep_start(HP03_ADC_ADDR_R);
	*v = i2c_readAck() << 8 | i2c_readNak();
	i2c_stop();
}

void HP03_Read(int *t, int *p) {

	u16 D1, D2;
	HP03_ReadOne(0xE8, &D2);
	HP03_ReadOne(0xF0, &D1);

	long Z = (D2 < HP03_Cal.Coeff.C5) ? HP03_Cal.Coeff.B : HP03_Cal.Coeff.A;
	long X = ((long) D2) - HP03_Cal.Coeff.C5;
	long dUT1 = (X * Z * X) >> 14;
	long dUT = (X - (dUT1 / (1 << HP03_Cal.Coeff.C)));

	*t = (250 + ((dUT * HP03_Cal.Coeff.C6) >> 16)) - (dUT >> HP03_Cal.Coeff.D);

	long Offs = (HP03_Cal.Coeff.C2 + (((HP03_Cal.Coeff.C4 - 1024) * dUT) >> 14))
			<< 2;
	long Sens = HP03_Cal.Coeff.C1 + ((HP03_Cal.Coeff.C3 * dUT) >> 10);
	X = ((Sens * (((long) D1) - 7168)) >> 14) - Offs;
	*p = ((X >> 5) + (((long) HP03_Cal.Coeff.C7) / 10));
}

ISR (TIMER1_CAPT_vect) {
	TCNT1 = 0;
}

void send() {
	int t = 0;
	int p = 0;

	HP03_Read(&t, &p);
	_delay_ms(1);

	HP03_Read(&t, &p);

	long freq = F_CPU / (ICR1);
	int RHtemp = ((HH10_Cal.off - freq) * HH10_Cal.sens) >> 12;

	RHcount++;
	if (RHcount > 15) {
		RHcount = 0;
	}
	RH[RHcount] = RHtemp;

	long RHsumme = 0;
	for (int i = 0; i < 16; i++) {
		RHsumme += RH[i];
	}
	int RHvalue = RHsumme>>4;

	char buf[32]; // = "g123456789012345678901234567890\0\0\0";
	buf[0] = 'g';
	memcpy(buf + 1, &RHvalue, 2);
	memcpy(buf + 3, &t, 2);
	memcpy(buf + 5, &p, 2);
	buf[7] = 0;
	memcpy(buf + 8, &RHvalue, 2);
	memcpy(buf + 10, &t, 2);
	memcpy(buf + 12, &p, 2);
	buf[14] = 0;

	rf12_init();
	rf12_setfreq(RF12FREQ868(868.3));
	rf12_setbandwidth(4, 1, 4);
	rf12_setbaud(666);
	rf12_setpower(0, 6);

	rf12_txdata(buf, 15);

	rf12_trans(0x8201);
	rf12_trans(0x0);

	_delay_ms(200);
}

int main() {

	uart_puts("Boot.......\r\n");

	rf12_preinit(AIRID);

	send();

	cli();
	wdt_reset();
	wdt_enable(WDTO_1S);
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	sei();

	TCCR1B = (1 << CS10);
	TCCR1A |= (1 << COM1A0);
	OCR1A = 183;

	//TCCR1B = (1<<ICES1) | (0<<WGM13);
	TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
	DDRD &= ~(1 << PD6);
	PORTD &= ~(1 << PD6);
//	TIFR |= (1<<ICF1);
	TIMSK |= (1 << ICIE1);

	uart_init(UART_BAUD_SELECT(57600, F_CPU));
	sei();

//	DDRD |= (1 << PD7);
//
//	DDRC |= (1 << PC2);
//	PORTC |= (1 << PC0) | (1 << PC1);

//	PORTC &= ~(1 << PC2);

	i2c_init();

	i2c_start_wait(HP03_CAL_ADDR_W);
	i2c_write(0x10);
	i2c_rep_start(HP03_CAL_ADDR_R);

	for (int i = 0; i < 7; i++) {
		HP03_Cal.bytes[(i << 1) + 1] = i2c_readAck();
		HP03_Cal.bytes[(i << 1)] = i2c_readAck();
	}
	for (int i = 14; i < 17; i++) {
		HP03_Cal.bytes[i] = i2c_readAck();
	}
	HP03_Cal.bytes[17] = i2c_readNak();
	i2c_stop();

	i2c_start_wait(HH10_CAL_ADDR_W);
	i2c_write(0x0A);
	i2c_rep_start(HH10_CAL_ADDR_R);
	HH10_Cal.sens = (i2c_readAck() << 8) | i2c_readAck();
	HH10_Cal.off = (i2c_readAck() << 8) | i2c_readNak();
	i2c_stop();

//	PORTC |= (1 << PC2); // XCLR high
	_delay_ms(10);

	for (; RHcount < 16; RHcount++) {
		RH[RHcount] = 0;
	}

	while (1) {

		if (WDTcounter >= MAXCOUNT)
		{
			send();
			WDTcounter = 0;
		}

		wdt_reset();

		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();

		sleep_cpu();
		sleep_disable();
		wdt_reset();
	}
}