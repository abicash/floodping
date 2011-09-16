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
	g_cPWMr = uiR; //LinearizeForEye(uiR) * 0.85;
	g_cPWMg = uiG; //LinearizeForEye(uiG) * 1;
	g_cPWMb = uiB; //LinearizeForEye(uiB) * 0.75;
	OCR1BL = g_cPWMb;
	OCR1AL = g_cPWMr;
	OCR2 = g_cPWMg;
}

void read_distance(int addr, int *puiLightCmd, int *puiTempLight) {
	uint8_t i2c_rtc_status;
	i2c_master_start_wait(addr + I2C_WRITE);
	if (i2c_master_write(0, &i2c_rtc_status) == 0) {
		if (i2c_master_rep_start(addr + I2C_READ
		, &i2c_rtc_status) == 0) {
			*puiLightCmd = i2c_master_read_ack();
			if (*puiTempLight == 0) {
				*puiTempLight = i2c_master_read_nak();
			} else {
				i2c_master_read_nak();
			}
			i2c_master_stop();

			if (*puiLightCmd > 0) {
				i2c_master_start_wait(addr + I2C_WRITE);
				i2c_master_write(3, &i2c_rtc_status);
				i2c_master_write(0xaa, &i2c_rtc_status);
				i2c_master_stop();
			}
		}
	}
}

int uiLastCmd = 0;
int uiLastCmdTimer = 0;

#define         SLAVE_ADDR_IR        0b00110100
#define         SLAVE_ADDR_DISTANCE1  0b00101100
#define         SLAVE_ADDR_DISTANCE2  0b00100100

int main() {
	wdt_reset();
	wdt_enable(WDTO_2S);
	wdt_reset();

	InitPWM();

	PORTC &= ~(1 << PC1);
	DDRC = (1 << PC1);

	if (MCUCSR & (1 << WDRF)) {
		SetColor(0xff, 0, 0);
		_delay_ms(400);
		SetColor(0, 0, 0);
		MCUCSR &= ~(1 << WDRF);
	} else {
		SetColor(0, 0xff, 0);
		_delay_ms(400);
		SetColor(0, 0, 0);
	}
	wdt_reset();

	PORTC |= (1 << PC1);
	_delay_ms(100);

	wdt_reset();

	cli();

	i2c_master_init();

	SetColor(0, 0xff, 0xff);

	sei();

	uint8_t i2c_errorcode, i2c_status;
	if (!i2c_rtc_init(&i2c_errorcode, &i2c_status)) {
		while (1) {
			SetColor(0, 0, 0xff);
			_delay_ms(100);
			SetColor(0, 0, 0);
			_delay_ms(100);
		};
	}

	int uiLight = 0;
	int uiLightFunction = 0;
	int uiLightPercent = 100;
	int uiLightCmd = 0;
	int uiTempLight = 0;

	unsigned char uiFunctionX = 0;

	int uiR = 0xff;
	int uiG = 0xff;
	int uiB = 0xff;

	while (1) {

		uiFunctionX++;

		wdt_reset();
		_delay_ms(50);


		uiTempLight = 0;
		read_distance(SLAVE_ADDR_DISTANCE1, &uiLightCmd, &uiTempLight);
		switch (uiLightCmd) {
		case 1:
			uiLight = 0;
			break;
		case 2:
			uiLight = 1;
			break;
		}

		read_distance(SLAVE_ADDR_DISTANCE2, &uiLightCmd, &uiTempLight);
		switch (uiLightCmd) {
		case 1:
			uiLight = 0;
			break;
		case 2:
			uiLight = 1;
			break;
		}

		uint8_t i2c_rtc_status;
		i2c_master_start_wait(SLAVE_ADDR_IR + I2C_WRITE);
		if (i2c_master_write(0, &i2c_rtc_status) == 0) {
			if (i2c_master_rep_start(SLAVE_ADDR_IR + I2C_READ
			, &i2c_rtc_status) == 0) {
				int command = (i2c_master_read_ack() << 8)
						| i2c_master_read_ack();
				int addr = (i2c_master_read_ack() << 8) | i2c_master_read_nak();
				i2c_master_stop();

				if (addr == 0b1110111100000000) {
					i2c_master_start_wait(SLAVE_ADDR_IR + I2C_WRITE);
					i2c_master_write(3, &i2c_rtc_status);
					i2c_master_write(0xaa, &i2c_rtc_status);
					i2c_master_stop();
					switch (command) {
					case 0:
						uiLightPercent += 2;
						if (uiLightPercent > 100) {
							uiLightPercent = 100;
						}
						break;
					case 1:
						uiLightPercent -= 2;
						if (uiLightPercent < 5) {
							uiLightPercent = 5;
						}
						break;
					case 2:
						uiLight = 0;
						break;
					case 3:
						uiLightFunction = 0;
						uiLight = 1;
						break;
					case 4:
						uiLightFunction = 0;
						uiLightPercent = 100;
						uiR = 0xff;
						uiG = 0x0;
						uiB = 0x0;
						break;
					case 5:
						uiLightFunction = 0;
						uiLightPercent = 100;
						uiR = 0x0;
						uiG = 0xff;
						uiB = 0x0;
						break;
					case 6:
						uiLightFunction = 0;
						uiLightPercent = 100;
						uiR = 0x0;
						uiG = 0x0;
						uiB = 0xff;
						break;
					case 7:
						uiLightFunction = 0;
						uiLightPercent = 100;
						uiR = 0xff;
						uiG = 0xff;
						uiB = 0xbf;
						break;
					case 11:
						uiLightFunction = 1;
						break;
					case 15:
						uiLightFunction = 2;
						break;
					case 19:
						uiLightFunction = 3;
						break;
					case 23:
						uiLightFunction = 4;
						break;
					}
				}
			}
		}

		switch (uiTempLight) {
		case 1:
			SetColor(0x22, 0x22, 0x22);
			break;
		case 2:
			SetColor(0xa0, 0xa0, 0xa0);
			break;
		case 3:
			SetColor(0x00, 0x00, 0xff);
			break;
		case 4:
			SetColor(0x00, 0xff, 0xff);
			break;
		case 5:
			SetColor(0xff, 0x00, 0xff);
			break;
		case 6:
			SetColor(0xff, 0xff, 0x00);
			break;
		default:
			if (uiLight == 0) {
				SetColor(0, 0, 0);
			} else if (uiLight == 1) {
				int dFunctionFactor = 1;
//				float dTemp;
				switch (uiLightFunction) {
				case 1:
					dFunctionFactor = ((uiFunctionX % 10) == 0) ? 1 : 0;
					break;
				case 2:
					dFunctionFactor = ((uiFunctionX % 2) == 0) ? 1 : 0;
					break;
//				case 3:
//					dTemp = ((uiFunctionX * 1.0) / 128);
//					dFunctionFactor = (dTemp > 1) ? 2 - dTemp : dTemp;
//					break;
//				case 4:
//					dFunctionFactor = ((uiFunctionX * 1.0) / 256);
//					break;
				}
				SetColor(dFunctionFactor * uiR * uiLightPercent / 100,
						dFunctionFactor * uiG * uiLightPercent / 100,
						dFunctionFactor * uiB * uiLightPercent / 100);
			}
		}
	}
	return 0;
}