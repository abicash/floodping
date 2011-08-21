#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "i2c-master.h"
#include "i2c-rtc.h"

void InitPWM() {
	DDRB = (1 << PB1) | (1 << PB2) | (1 << PB3);
	TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
	TCCR1B = (1 << CS10);
	TCCR2 = (1 << CS20) | (1 << WGM20) | (1 << COM21);
}

uint8_t LinearizeForEye(uint8_t x) {
	//	if (x >= 0 && x < 5) {
	return (x);
	//	} else if (x >= 5 && x < 50) {
	//		return (x / 5);
	//	}
	//	return (((uint16_t) x) * x) >> 8;
}

char g_cPWMr = 0;
char g_cPWMg = 0;
char g_cPWMb = 0;

void SetColor(uint8_t uiR, uint8_t uiG, uint8_t uiB) {
	g_cPWMr = LinearizeForEye(uiR) * 0.85;
	g_cPWMg = LinearizeForEye(uiG) * 1;
	g_cPWMb = LinearizeForEye(uiB) * 0.75;
	OCR1BL = g_cPWMr;
	OCR1AL = g_cPWMg;
	OCR2 = g_cPWMb;
}

uint16_t AdcRead(uint8_t channel) {
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)) {
	}
	return ADCW;
}

int uiLastCmd = 0;
int uiLastCmdTimer = 0;
volatile int uiX = 0;
ISR(INT0_vect) {
	uiX = uiX == 0 ? 0xff : 0;
	SetColor(uiX, 0x0, 0);
}

#define         SLAVE_ADDR_ATTINY       0b00110100

int main() {
	wdt_reset();
	WDTCR |= (1 << WDP0) | (1 << WDP1) | (1 << WDP2);
	InitPWM();

	i2c_master_init();

//	IRMP_DATA irmp_data;
//
//	irmp_init(); // initialize rc5
//	timer_init();

//	MCUCR |= (1 << ISC00);
//	GIMSK |= (1<< INT0);
	sei();
	// enable interrupts

	_delay_ms(100);
	wdt_reset();
	SetColor(0xff, 0, 0);
	_delay_ms(100);
	wdt_reset();
	SetColor(0, 0xff, 0);
	_delay_ms(100);
	wdt_reset();
	SetColor(0, 0, 0xff);
	_delay_ms(100);
	wdt_reset();

	uint8_t i2c_errorcode, i2c_status;
	if (!i2c_rtc_init(&i2c_errorcode, &i2c_status)) // initialize rtc
			{
		SetColor(0xff, 0, 0);
		while (1)
			;
	}

//	while (1) {
////		int res1 = i2c_rtc_read(&time, 1);
////		if (res1) {
////			SetColor(0, ((time.ss & 1)==1)?0xff:0x80, 0);
////		}
//
//		i2c_master_start_wait(SLAVE_ADDR_ATTINY + I2C_WRITE); // set device address and write mode
//
//		uint8_t i2c_rtc_status;
//		if (i2c_master_write(0, &i2c_rtc_status) == 0) // write address
//				{
//			if (i2c_master_rep_start(SLAVE_ADDR_ATTINY + I2C_READ
//					, &i2c_rtc_status) == 0) // set device address and read mode
//					{
//				int addr0 = i2c_master_read_ack();
//				int addr1 = i2c_master_read_ack();
//				int command = (i2c_master_read_ack() << 8)
//						| i2c_master_read_ack();
//				int addr = (i2c_master_read_ack() << 8) | i2c_master_read_ack();
//				int counter = i2c_master_read_nak();
//
//				// addr: 0000000011110111
//
//				if (addr == 0b1110111100000000) {
//					switch (command) {
//					case 0:
//						break;
//					case 1:
//						break;
//					case 2:
//						SetColor(0, 0, 0);
//						break;
//					case 3:
//						SetColor(0xff, 0xff, 0xff);
//						break;
//					}
//				}
//			}
//		}
//		i2c_master_stop(); // set stop conditon = release bus
//	}

//		  while(1)
//	{
//		i2c_start_wait(SLAVE_ADDR_ATTINY+I2C_WRITE);
//		i2c_write(0);
//
//		i2c_rep_start(SLAVE_ADDR_ATTINY+I2C_READ);
//		int buffer1=i2c_readNak();
//		int buffer2=i2c_readNak();
//		int buffer3=i2c_readNak();
//		int buffer4=i2c_readNak();
//		i2c_stop();
//
//		SetColor(buffer2, buffer3, buffer4);
//		_delay_ms(1000);
//		wdt_reset();
//	}

//	DDRB |= (1 << PB1) | (1 << PB2) | (1 << PB3);

	ADMUX |= (1 << REFS1) | (1 << REFS0) | (1 << ADLAR);
	ADCSRA |= (1 << ADEN); // | (1 << ADFR);
	ADCSRA |= (1 << ADSC) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);

	int uiLastDistance = 0;
	int uiMovement = 0;
	int uiLight = 0;
	int uiTempLight = 0;

	while (1) {
//		PORTB = (1 << PB1) | (1 << PB2) | (1 << PB3);
//		_delay_ms(510);
//		wdt_reset();
//		PORTB = 0;

		wdt_reset();
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

		int uiR, uiG, uiB;
		if (uiMovement > 5) {
			switch (uiLastCmd) {
			case 1:
				break;
			case 2:
				uiLight = 0;
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
				uiLight = 1;
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

		i2c_master_start_wait(SLAVE_ADDR_ATTINY + I2C_WRITE); // set device address and write mode

		uint8_t i2c_rtc_status;
		if (i2c_master_write(0, &i2c_rtc_status) == 0) // write address
				{
			if (i2c_master_rep_start(SLAVE_ADDR_ATTINY + I2C_READ
			, &i2c_rtc_status) == 0) // set device address and read mode
					{
				int addr0 = i2c_master_read_ack();
				int addr1 = i2c_master_read_ack();
				int command = (i2c_master_read_ack() << 8)
						| i2c_master_read_ack();
				int addr = (i2c_master_read_ack() << 8) | i2c_master_read_ack();
				int counter = i2c_master_read_nak();

				if (addr == 0b1110111100000000) {
					switch (command) {
					case 0:
						break;
					case 1:
						break;
					case 2:
						uiLight = 0;
						break;
					case 3:
						uiLight = 1;
						break;
					}
				}
			}
		}
		i2c_master_stop(); // set stop conditon = release bus

		if (uiTempLight == 3) {
			SetColor(0x00, 0xff, 0x00);
		} else if (uiTempLight == 4) {
			SetColor(0xff, 0x00, 0x00);
		} else if (uiTempLight == 5) {
			SetColor(0x4, 0, 0);
//			_delay_ms(1);
//			SetColor(0, 0, 0);
		} else if (uiTempLight == 6) {
//			SetColor(0, 0xff, 0);
//			_delay_ms(24);
			SetColor(0x7f, 0xff, 0x7f);
		} else if (uiLight == 0) {
			SetColor(0, 0, 0);
		} else if (uiLight == 1) {
			SetColor(0xff, 0xff, 0xb0);
		}

	}
}
