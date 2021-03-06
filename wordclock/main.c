#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "ldr.h"
#include "i2c-rtc.h"

#ifndef cbi
#define cbi(sfr, bit)     (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit)     (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void
InitPWM()
{
  DDRD = 0xfc;
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(CS10);
  TCCR2 = _BV(CS20) | _BV(WGM20) | _BV(COM21);
  TCCR0 = _BV(WGM01) | _BV(WGM00) | _BV(COM01) | _BV(COM00) | _BV(CS00) | _BV(CS02);
  TIMSK &= ~0x3c;
}

uint8_t
LinearizeForEye(uint8_t x)
{
  if (x >= 0 && x < 5)
  {
    return (x);
  }
  else if (x >= 5 && x < 50)
  {
    return (x / 5);
  }
  return (((uint16_t) x) * x) >> 8;
}

char g_cPWMr = 0;
char g_cPWMg = 0;
char g_cPWMb = 0;

void
SetColor(uint8_t bright, uint8_t uiR, uint8_t uiG, uint8_t uiB)
{
  g_cPWMr = ((((uint16_t) bright) * LinearizeForEye(uiR)) / 255);
  g_cPWMg = ((((uint16_t) bright) * LinearizeForEye(uiG)) / 255);
  g_cPWMb = ((((uint16_t) bright) * LinearizeForEye(uiB)) / 255);
  OCR1BL = g_cPWMr;
  OCR1AL = g_cPWMg;
  OCR2 = g_cPWMb;
}

void
uartPutc(char c)
{
  while (!(UCSRA & _BV(UDRE)))
    ;
  UDR = c;
}

void
uartPuts(char *s)
{
  int x = 0;
  while (s[x])
  {
    uartPutc(s[x]);
    x++;
  }
}

#define SHIFT_SR_SPI_DDR  DDRB
#define SHIFT_SR_SPI_PORT PORTB
#define SHIFT_SR_SPI_MOSI PIN5
#define SHIFT_SR_SPI_MISO PIN6
#define SHIFT_SR_SPI_RCLK PIN4
#define SHIFT_SR_SPI_SCK  PIN7

uint32_t lLEDs_LastValue = 0;

void
shift32_output(uint32_t value)
{
  if (value == lLEDs_LastValue)
  {
    return;
  }
  lLEDs_LastValue = value;
  uint8_t u0 = (uint8_t) (value);
  uint8_t u1 = (uint8_t) (value >> 8);
  uint8_t u2 = (uint8_t) (value >> 16);
  uint8_t u3 = (uint8_t) (value >> 24);

  SPDR = u3;
  while (!(SPSR & _BV(SPIF)))
    ;

  SPDR = u2;
  while (!(SPSR & _BV(SPIF)))
    ;

  SPDR = u1;
  while (!(SPSR & _BV(SPIF)))
    ;

  SPDR = u0;
  while (!(SPSR & _BV(SPIF)))
    ;

  /* latch data */
  SHIFT_SR_SPI_PORT &= ~_BV(SHIFT_SR_SPI_RCLK);
  SHIFT_SR_SPI_PORT |= _BV(SHIFT_SR_SPI_RCLK);
}

void
shift_init(void)
{
  SHIFT_SR_SPI_DDR |= _BV(SHIFT_SR_SPI_MOSI) | _BV(SHIFT_SR_SPI_RCLK) | _BV(SHIFT_SR_SPI_SCK);
  SHIFT_SR_SPI_DDR &= ~_BV(SHIFT_SR_SPI_MISO); /* MISO muss eingang sein */
  SHIFT_SR_SPI_PORT |= _BV(SHIFT_SR_SPI_RCLK) | _BV(SHIFT_SR_SPI_MISO);

  SPCR = _BV(SPE) | _BV(MSTR) | _BV(CPOL) | _BV(SPR0) | _BV(SPR1);

  SPSR |= _BV(SPI2X);

  shift32_output(0xFFFFFFFF);
}

#define UART_MAXSTRLEN 100

volatile uint8_t uart_str_complete = 0;
volatile uint8_t uart_str_count = 0;
volatile char uart_string[UART_MAXSTRLEN + 1] = "";

ISR(SIG_UART_RECV)
{
  unsigned char nextChar;

  nextChar = UDR;
  if (uart_str_complete == 0)
  {
    if (nextChar != '\n' && nextChar != '\r' && uart_str_count < UART_MAXSTRLEN - 1)
    {
      uart_string[uart_str_count] = nextChar;
      uart_str_count++;
    }
    else
    {
      uart_string[uart_str_count] = '\0';
      uart_str_count = 0;
      uart_str_complete = 1;
    }
  }
}

int
hex2dez_c(char h)
{
  int res = -1;
  if (h >= '0' && h <= '9')
  {
    res = (h - '0');
  }
  else if (h >= 'A' && h <= 'F')
  {
    res = (h - 'A' + 10);
  }
  else if (h >= 'a' && h <= 'f')
  {
    res = (h - 'a' + 10);
  }
  return res;
}

int
hex2dez(char *h)
{
  int res1 = hex2dez_c(h[0]);
  int res2 = hex2dez_c(h[1]);
  if (res1 < 0 || res2 < 0)
    return -1;
  return (res1 << 4) + res2;
}

#define def_es       0
#define def_ist      1
#define def_fuenfM   2
#define def_zehnM    3
#define def_zwanzigM 4
#define def_dreiM    5
#define def_viertelM 6
#define def_nach     7
#define def_vor      8
#define def_halb     9
#define def_zwoelfH  10
#define def_zweiH    11
#define def_einsH    12
#define def_siebenH  13
#define def_dreiH    14
#define def_fuenfH   15
#define def_elfH     16
#define def_neunH    17
#define def_vierH    18
#define def_achtH    19
#define def_zehnH    20
#define def_sechsH   21
#define def_uhr      22
#define def_mp1      23
#define def_mp2      24
#define def_mp3      25
#define def_mp4      26

const long words[30] PROGMEM =
  { 0b00000000000000000000000000000001, // es
      0b00000000000000000000000000000010, // ist
      0b00000000000000000000000000000100, // fünf
      0b00000000000000000000000000001000, // zehn
      0b00000000000000000000000000010000, // zwanzig
      0b00000000000000000000000000100000, // drei
      0b00000000000000000000000001000000, // viertel
      0b00000000000000000000000010000000, // nach
      0b00000000000000000000000100000000, // vor
      0b00000000000000000000001000000000, // halb
      0b00000000000000000000010000000000, // zwölf
      0b00000000000000000001100000000000, // zwei
      0b00000000000000000011000000000000, // ein
      0b00000000000000000100000000000000, // sieben
      0b00000000000000001000000000000000, // drei
      0b00000000000000010000000000000000, // fünf
      0b00000000000000100000000000000000, // elf
      0b00000000000001000000000000000000, // neun
      0b00000000000010000000000000000000, // vier
      0b00000000000100000000000000000000, // acht
      0b00000000001000000000000000000000, // zehn
      0b00000000010000000000000000000000, // sechs
      0b00000000100000000000000000000000, // uhr
      0b00000001000000000000000000000000, // m+1
      0b00000010000000000000000000000000, // m+2
      0b00000100000000000000000000000000, // m+3
      0b00001000000000000000000000000000, // m+4
      0b00010000000000000000000000000000, //
      0b00100000000000000000000000000000, //
      0b01000000000000000000000000000000 //
    };

void
TimeInfo(DATETIME time, int bSun, int bInitformat)
{
  char s[100];
  if (!bInitformat)
  {
    sprintf(s, "NOW:  %02d:%02d:%02d %s %02d.%02d.%4d UTC%+d %s\r\n", time.hh, time.mm, time.ss, "x", time.DD, time.MM, time.YY + 2000, rtc_offset, time.dst != 0 ? "DST" : "");
    uartPuts(s);
    if (bSun)
    {
      sprintf(s, "Sun: %%%d %02d:%02d - %02d:%02d\r\n", time.sunrise, time.sunrisehh, time.sunrisemm, time.sunfallhh, time.sunfallmm);
      uartPuts(s);
    }
  }
  else
  {
    sprintf(s, " %02d:%02d:%02d %s %02d.%02d.%4d UTC%+d %s", time.hh, time.mm, time.ss, "x", time.DD, time.MM, time.YY + 2000, rtc_offset, time.dst != 0 ? "DST" : "");
    uartPuts(s);
  }
}

int
main()
{
  wdt_reset();
  cli();
  UCSRB |= _BV(TXEN) | _BV(RXEN) | _BV(RXCIE);
  UCSRC |= _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);

  PORTC &= ~(1 << PC7);
  DDRC |= (1 << PC7);

  UBRRH = 0x00;
  UBRRL = 95;

  uartPuts("at\r");
  _delay_ms(100);
  uartPuts("atl5\r");
  _delay_ms(100);

  UBRRH = 0x00;
  UBRRL = 0x07;

  uartPuts("\r\n\r\n\r\n\r\n\r\n\r\n\r\nWordClock V0.1 initializing...\r\n\r\n");

  uartPuts("\r\nReset-Cause:");
  switch (MCUCSR & 0x1f)
  {
  case 1:
    uartPuts("Power-On Reset\r\n");
    break;
  case 2:
    uartPuts("External Reset\r\n");
    break;
  case 4:
    uartPuts("Brown-Out Reset\r\n");
    break;
  case 8:
    uartPuts("Watchdog Reset\r\n");
    break;
  case 16:
    uartPuts("JTAG Reset\r\n");
    break;
  default:
    uartPuts("unknown\r\n");
    break;
  }
  MCUCSR = 0;

  uartPuts("\r\n... switch on RTC");

  for (int cyx = 0; cyx < 50; cyx++)
  {
    wdt_reset();
    _delay_ms(10);
  }
  PORTC |= (1 << PC7);
  for (int cyx = 0; cyx < 50; cyx++)
  {
    wdt_reset();
    _delay_ms(10);
  }

  uartPuts("\r\n... WDT enable");
  WDTCR = _BV(WDE) | 0b101;

  sei();

  uartPuts("\r\n... PWM");
  InitPWM();
  SetColor(0xF, 0xFF, 0x00, 0x00);

  wdt_reset();

  uartPuts("\r\n... Shifter");
  shift_init();
  SetColor(0xF, 0xFF, 0xFF, 0x00);

  wdt_reset();

  uartPuts("\r\n... LDR ADC");
  ldr_init();
  SetColor(0xF, 0x00, 0xFF, 0xFF);

  wdt_reset();

  uartPuts("\r\n... RTC");
  DATETIME time;
  DATETIME utctime;
  uint8_t i2c_errorcode;
  uint8_t i2c_status;
  if (!i2c_rtc_init(&i2c_errorcode, &i2c_status)) // initialize rtc
  {
    uartPuts(" FAILED !!!\r\n");
    for (;;)
    {
      SetColor(0xF, 0x00, 0x00, 0x00);
      _delay_ms(20);
      SetColor(0xF, 0xff, 0xff, 0xff);
      _delay_ms(20);
    }
  }
  wdt_reset();
  int res1 = i2c_rtc_read(&time, 1);
  if (res1)
  {
    TimeInfo(time, 0, 1);
  }
  else
  {
    uartPuts(" ... RTC error!!!\r\n");
  }

  uartPuts("\r\n... LED Check");
  shift32_output(0);
  SetColor(0xFF, 0xFF, 0xFF, 0xFF);

  wdt_reset();

  unsigned long uiScrollingBit = 0x80000000;
  while (uiScrollingBit)
  {
    wdt_reset();
    shift32_output(uiScrollingBit);
    uiScrollingBit >>= 1;
    _delay_ms(20);
  }
  shift32_output(uiScrollingBit);
  for (int cyx = 0; cyx < 50; cyx++)
  {
    wdt_reset();
    _delay_ms(10);
  }
  uiScrollingBit = 1;
  while (uiScrollingBit)
  {
    wdt_reset();
    shift32_output(uiScrollingBit);
    uiScrollingBit <<= 1;
    _delay_ms(20);
  }

  wdt_reset();

  uartPuts("\r\n... RGB Check");
  shift32_output(0xffffffff);
  SetColor(0x7F, 0xFF, 0x00, 0x00);
  for (int cyx = 0; cyx < 20; cyx++)
  {
    wdt_reset();
    _delay_ms(10);
  }
  SetColor(0x7F, 0x00, 0xFF, 0x00);
  for (int cyx = 0; cyx < 20; cyx++)
  {
    wdt_reset();
    _delay_ms(10);
  }
  SetColor(0x7F, 0x00, 0x00, 0xFF);
  for (int cyx = 0; cyx < 20; cyx++)
  {
    wdt_reset();
    _delay_ms(10);
  }
  SetColor(0x00, 0xFF, 0xFF, 0xFF);

  wdt_reset();

  int8_t uiBrightControl = 1;
  read_byte(cBrightControl, &uiBrightControl);

  char s[100];
  sprintf(s, "%d", uiBrightControl);
  uartPuts("\r\n... Brightness:");
  uartPuts(s);

  unsigned char uiR;
  unsigned char uiG;
  unsigned char uiB;
  unsigned char uiRGB;
  read_byte(cRGB_R, &uiR);
  read_byte(cRGB_G, &uiG);
  read_byte(cRGB_B, &uiB);
  read_byte(cRGB_Mode, &uiRGB);

  if (!uiRGB)
  {
    sprintf(s, "\r\n... RGB color: #%02x%02x%02x", uiR, uiG, uiB);
    uartPuts(s);
  }
  else
  {
    uartPuts("\r\n... RGB Auto");
  }

  uartPuts("\r\nReady...\r\n");

  long uiCount = -1;

  uint16_t Button1 = 0;
  uint16_t Button2 = 0;
  uint16_t Button3 = 0;

  while (1)
  {
    wdt_reset();
    _delay_ms(1);
    uiCount++;

    // alle 1ms

    ldr_read();

    uint8_t uiBright = 0xff;
    if (uiBrightControl < 0)
    {
      uiBright = 0;
    }
    else if (uiBrightControl > 0)
    {
		uiBright = ldr_get_brightness();
	    if (uiBright < 128)
	    {
	      uiBright = (uiBright >> 1) + 64;
	    }
    }
    SetColor(uiBright, uiR, uiG, uiB);

    if (uiCount % 10)
    {
      continue;
    }

    // ca. alle 10ms

    if (uart_str_complete)
    {
      if (uart_string[0] == '>')
      {
        switch (uart_string[1])
        {
        case '?':
          {
            char s[200];
            sprintf(s, "\r\nBrightness: %d\r\nPWM-Color: #%02x%02x%02x\r\n", uiBright, g_cPWMr, g_cPWMg, g_cPWMb);
            uartPuts(s);
          }
          break;
        case 'p':
          uartPuts("\r\nParty-Mode...\r\n");
          shift32_output(0xffffffff);
          uart_str_complete = 0;
          while (!uart_str_complete)
          {
            SetColor(0xFF, 0x80, 0x00, 0x00);
            wdt_reset();
            _delay_ms(20);
            SetColor(0xFF, 0xFF, 0x00, 0x00);
            wdt_reset();
            _delay_ms(200);
            SetColor(0xFF, 0x00, 0x80, 0x00);
            wdt_reset();
            _delay_ms(20);
            SetColor(0xFF, 0x00, 0xFF, 0x00);
            wdt_reset();
            _delay_ms(200);
            SetColor(0xFF, 0x00, 0x00, 0x80);
            wdt_reset();
            _delay_ms(20);
            SetColor(0xFF, 0x00, 0x00, 0xFF);
            wdt_reset();
            _delay_ms(200);
            SetColor(0xFF, 0xFF, 0xFF, 0xFF);
            wdt_reset();
            _delay_ms(200);
          }
          wdt_reset();
          SetColor(0, 0, 0, 0);
          break;
        case 'q':
          {
            long q1 = hex2dez((char*) &uart_string[2]);
            q1 <<= 8;
            q1 |= hex2dez((char*) &uart_string[4]);
            q1 <<= 8;
            q1 |= hex2dez((char*) &uart_string[6]);
            q1 <<= 8;
            q1 |= hex2dez((char*) &uart_string[8]);
            shift32_output(q1);
            uart_str_complete = 0;
            while (!uart_str_complete)
            {
              wdt_reset();
              _delay_ms(200);
            }
            SetColor(0, 0, 0, 0);
          }
          break;
        case 'l':
          uartPuts("\r\nLight-Mode...\r\n");
          shift32_output(0xffffffff);
          uart_str_complete = 0;
          SetColor(0xFF, 0xFF, 0xFF, 0xFF);
          while (!uart_str_complete)
          {
            wdt_reset();
            _delay_ms(20);
          }
          wdt_reset();
          SetColor(0, 0, 0, 0);
          lLEDs_LastValue = 0xffffffff;
          break;
        case 'r':
          uartPuts("\r\nRebooting via Watchdog...\r\n");
          while (1)
            ;
          break;
        case 'b':
          {
            switch (uart_string[2])
            {
            case '+':
              {
                uiBrightControl = 1;
                save_byte(cBrightControl, uiBrightControl);
                char s[100];
                sprintf(s, "brightness control is %s\r\n", uiBrightControl != 0 ? "active" : "inactive");
                uartPuts(s);
              }
              break;
            case '-':
              {
                uiBrightControl = 0;
                save_byte(cBrightControl, uiBrightControl);
                char s[100];
                sprintf(s, "brightness control is %s\r\n", uiBrightControl != 0 ? "active" : "inactive");
                uartPuts(s);
              }
              break;
            default:
              {
                uiBrightControl = uiBrightControl != 0 ? 0 : 1;
                save_byte(cBrightControl, uiBrightControl);
                char s[100];
                sprintf(s, "brightness control is %s\r\n", uiBrightControl > 0 ? "active" : "inactive");
                uartPuts(s);
              }
              break;
            }
          }
          break;
        case 'o':
          {
            uiBrightControl = -1;
            save_byte(cBrightControl, uiBrightControl);
            char s[100];
            sprintf(s, "brightness control is %s\r\n", uiBrightControl < 0 ? "off" : "other");
            uartPuts(s);
          }
          break;
        case 'c':
          {
            int r = hex2dez((char*) &uart_string[2]);
            int g = hex2dez((char*) &uart_string[4]);
            int b = hex2dez((char*) &uart_string[6]);
            if (r < 0 || g < 0 || b < 0)
            {
              if (uiRGB)
                uiRGB = 0;
              else
                uiRGB = 1;
              save_byte(cRGB_Mode, uiRGB);
              break;
            }
            uiRGB = 0;
            uiR = r;
            uiG = g;
            uiB = b;
            SetColor(uiBright, uiR, uiG, uiB);
            save_byte(cRGB_R, uiR);
            save_byte(cRGB_G, uiG);
            save_byte(cRGB_B, uiB);
            save_byte(cRGB_Mode, uiRGB);
          }
          break;
        case 'u':
          {
            int h = hex2dez((char*) &uart_string[2]);
            int m = hex2dez((char*) &uart_string[4]);
            int s = hex2dez((char*) &uart_string[6]);
            int D = hex2dez((char*) &uart_string[8]);
            int M = hex2dez((char*) &uart_string[10]);
            int Y = hex2dez((char*) &uart_string[12]);
            if (h < 0 || m < 0 || s < 0 || D < 0 || M < 0 || Y < 0)
            {
              break;
            }
            time.hh = h;
            time.mm = m;
            time.ss = s;
            time.DD = D;
            time.MM = M;
            time.YY = Y;
            i2c_rtc_write(&time);
          }
          break;
        case 't':
          {
            if (uiScrollingBit)
            {
              uiScrollingBit = 0;
            }
            else
            {
              uiScrollingBit = 0x80000000;
            }
          }
          break;
        case '+':
          {
            int res = i2c_rtc_read(&time, 0);
            add_minute(&time);
            res = i2c_rtc_write(&time);
          }
          break;
        case '-':
          {
            int res = i2c_rtc_read(&time, 0);
            sub_minute(&time);
            res = i2c_rtc_write(&time);
          }
          break;
        case 'z':
          switch (uart_string[2])
          {
          case '+':
            {
              if (rtc_offset > 11)
                rtc_offset = -13;
              set_offset(rtc_offset + 1);
            }
            break;
          case '-':
            {
              if (rtc_offset < -11)
                rtc_offset = 13;
              set_offset(rtc_offset - 1);
            }
            break;
          default:
            {
              char s[100];
              sprintf(s, "z macht keinen sinn mit '%c'...\r\n", uart_string[2]);
              uartPuts(s);
            }
            break;
          }
          break;
        case 'h':
          switch (uart_string[2])
          {
          case '+':
            {
              int res = i2c_rtc_read(&time, 0);
              add_hour(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          case '-':
            {
              int res = i2c_rtc_read(&time, 0);
              sub_hour(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          default:
            {
              char s[100];
              sprintf(s, "h macht keinen sinn mit '%c'...\r\n", uart_string[2]);
              uartPuts(s);
            }
            break;
          }
          break;
        case 'm':
          switch (uart_string[2])
          {
          case '+':
            {
              int res = i2c_rtc_read(&time, 0);
              add_minute(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          case '-':
            {
              int res = i2c_rtc_read(&time, 0);
              sub_minute(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          default:
            {
              char s[100];
              sprintf(s, "m macht keinen sinn mit '%c'...\r\n", uart_string[2]);
              uartPuts(s);

            }
            break;
          }
          break;
        case 'D':
          switch (uart_string[2])
          {
          case '+':
            {
              int res = i2c_rtc_read(&time, 0);
              add_day(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          case '-':
            {
              int res = i2c_rtc_read(&time, 0);
              sub_day(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          default:
            {
              char s[100];
              sprintf(s, "D macht keinen sinn mit '%c'...\r\n", uart_string[2]);
              uartPuts(s);
            }
            break;
          }
          break;
        case 'M':
          switch (uart_string[2])
          {
          case '+':
            {
              int res = i2c_rtc_read(&time, 0);
              add_month(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          case '-':
            {
              int res = i2c_rtc_read(&time, 0);
              sub_month(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          default:
            {
              char s[100];
              sprintf(s, "M macht keinen sinn mit '%c'...\r\n", uart_string[2]);
              uartPuts(s);
            }
            break;
          }
          break;
        case 'Y':
          switch (uart_string[2])
          {
          case '+':
            {
              int res = i2c_rtc_read(&time, 0);
              add_year(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          case '-':
            {
              int res = i2c_rtc_read(&time, 0);
              sub_year(&time);
              res = i2c_rtc_write(&time);
            }
            break;
          default:
            {
              char s[100];
              sprintf(s, "Y macht keinen sinn mit '%c'...\r\n", uart_string[2]);
              uartPuts(s);
            }
            break;
          }
          break;
        default:
          {
            char s[100];
            sprintf(s, "was soll ich mit '%s' anfangen?\r\n", uart_string);
            uartPuts(s);

          }
        }
      }
      int res1 = i2c_rtc_read(&time, 1);
      int res2 = i2c_rtc_read(&utctime, 0);
      if (!res1 || !res2)
      {
        uartPuts("RTC error\r\n");
        for (uiCount = 0; uiCount < 40; uiCount++)
        {
          SetColor(0x01, 0x01, 0x01, 0x01);
          _delay_ms(50);
          SetColor(0xff, 0xff, 0xff, 0xff);
          _delay_ms(50);
        }
        while (1)
          ;
      }
      TimeInfo(time, 1, 0);
      uart_str_complete = 0;
    }

    if (uiCount % 100)
    {
      continue;
    }

    // ca. alle 100ms

    if (!(PINA & _BV(PA7)))
    {
      Button1++;
    }
    else
    {
      Button1 = 0;
    }

    if (!(PINA & _BV(PA6)))
    {
      Button2++;
    }
    else
    {
      Button2 = 0;
    }

    if (!(PINA & _BV(PA6)))
    {
      Button3++;
    }
    else
    {
      Button3 = 0;
    }

    if (Button1 > 1)
    {
      if (((Button1 + 10) % 12) == 0 || Button1 > 36)
      {
        uartPuts("Hour++\r\n");
        int res = i2c_rtc_read(&time, 0);
        if (!res)
        {
          while (1)
            ;
        }
        add_hour(&time);
        res = i2c_rtc_write(&time);
        char s[100];
        sprintf(s, "time : %02d:%02d:%02d\r\n", time.hh, time.mm, time.ss);
        uartPuts(s);
      }
    }

    if (Button2 > 1)
    {
      if (((Button2 + 10) % 12) == 0 || Button2 > 36)
      {
        uartPuts("Min++\r\n");
        int res = i2c_rtc_read(&time, 0);
        if (!res)
        {
          while (1)
            ;
        }
        add_minute(&time);
        i2c_rtc_write(&time);
        char s[100];
        sprintf(s, "time : %02d:%02d:%02d\r\n", time.hh, time.mm, time.ss);
        uartPuts(s);
      }
    }

    if (uiScrollingBit)
    {
      uiScrollingBit >>= 1;
      if (!uiScrollingBit)
      {
        uiScrollingBit = 0x80000000;
      }
      shift32_output(uiScrollingBit);
    }

    if (uiCount % 1000)
    {
      continue;
    }

    // ca. alle 1s

    int res1 = i2c_rtc_read(&time, 1);
    int res2 = i2c_rtc_read(&utctime, 0);
    SetColor(uiBright, uiR, uiG, uiB);
    if (!res1 || !res2)
    {
      uartPuts("RTC error\r\n");
      for (uiCount = 0; uiCount < 40; uiCount++)
      {
        SetColor(0x01, 0x01, 0x01, 0x01);
        _delay_ms(50);
        SetColor(0xff, 0xff, 0xff, 0xff);
        _delay_ms(50);
      }
      while (1)
        ;
    }

    long lLEDs = 0;
    lLEDs |= pgm_read_dword(words+def_es);
    lLEDs |= pgm_read_dword(words+def_ist);
    lLEDs |= pgm_read_dword(words+def_uhr);

    switch (time.mm % 5)
    {
    case 0:
      break;
    case 1:
      lLEDs |= pgm_read_dword(words+def_mp4);
      break;
    case 2:
      lLEDs |= pgm_read_dword(words+def_mp4);
      lLEDs |= pgm_read_dword(words+def_mp2);
      break;
    case 3:
      lLEDs |= pgm_read_dword(words+def_mp4);
      lLEDs |= pgm_read_dword(words+def_mp2);
      lLEDs |= pgm_read_dword(words+def_mp1);
      break;
    case 4:
      lLEDs |= pgm_read_dword(words+def_mp4);
      lLEDs |= pgm_read_dword(words+def_mp2);
      lLEDs |= pgm_read_dword(words+def_mp1);
      lLEDs |= pgm_read_dword(words+def_mp3);
      break;
    }
    uint8_t hoffset = 0;
    if (time.mm < 5)
    {
    }
    else if (time.mm < 10)
    {
      lLEDs |= pgm_read_dword(words+def_fuenfM);
      lLEDs |= pgm_read_dword(words+def_nach);
    }
    else if (time.mm < 15)
    {
      lLEDs |= pgm_read_dword(words+def_zehnM);
      lLEDs |= pgm_read_dword(words+def_nach);
    }
    else if (time.mm < 20)
    {
      lLEDs |= pgm_read_dword(words+def_viertelM);
      lLEDs |= pgm_read_dword(words+def_nach);
    }
    else if (time.mm < 25)
    {
      lLEDs |= pgm_read_dword(words+def_zwanzigM);
      lLEDs |= pgm_read_dword(words+def_nach);
    }
    else if (time.mm < 30)
    {
      lLEDs |= pgm_read_dword(words+def_fuenfM);
      lLEDs |= pgm_read_dword(words+def_vor);
      lLEDs |= pgm_read_dword(words+def_halb);
      hoffset = 1;
    }
    else if (time.mm < 35)
    {
      lLEDs |= pgm_read_dword(words+def_halb);
      hoffset = 1;
    }
    else if (time.mm < 40)
    {
      lLEDs |= pgm_read_dword(words+def_fuenfM);
      lLEDs |= pgm_read_dword(words+def_nach);
      lLEDs |= pgm_read_dword(words+def_halb);
      hoffset = 1;
    }
    else if (time.mm < 45)
    {
      lLEDs |= pgm_read_dword(words+def_zwanzigM);
      lLEDs |= pgm_read_dword(words+def_vor);
      hoffset = 1;
    }
    else if (time.mm < 50)
    {
      lLEDs |= pgm_read_dword(words+def_viertelM);
      lLEDs |= pgm_read_dword(words+def_vor);
      hoffset = 1;
    }
    else if (time.mm < 55)
    {
      lLEDs |= pgm_read_dword(words+def_zehnM);
      lLEDs |= pgm_read_dword(words+def_vor);
      hoffset = 1;
    }
    else if (time.mm < 60)
    {
      lLEDs |= pgm_read_dword(words+def_fuenfM);
      lLEDs |= pgm_read_dword(words+def_vor);
      hoffset = 1;
    }

    int def_h[25] =
      { def_zwoelfH, def_einsH, def_zweiH, def_dreiH, def_vierH, def_fuenfH, def_sechsH, def_siebenH, def_achtH, def_neunH, def_zehnH, def_elfH, def_zwoelfH, def_einsH, def_zweiH, def_dreiH, def_vierH, def_fuenfH, def_sechsH, def_siebenH, def_achtH, def_neunH, def_zehnH, def_elfH, def_zwoelfH };
    lLEDs |= pgm_read_dword(words + def_h[time.hh + hoffset]);

    if (!uiScrollingBit)
    {
      shift32_output(lLEDs);
    }

    if (time.ss == 0)
    {
      TimeInfo(time, 0, 0);
    }

    if (uiRGB)
    {
      if (time.sunrise == 0)
      {
        uiR = 0x01;
        uiG = 0x01;
        uiB = 0x55;
      }
      else if (time.sunrise >= 100)
      {
	    if ( (uiBrightControl == -1) )
        {
          uiBrightControl = 1;
        }
	    
        uiR = 0xff;
        uiG = 0xff;
        uiB = 0x80;
      }
      else if (time.sunrise < 50)
      {
        uiR = 0x01 + (time.sunrise * 0xfe) / 50;
        uiG = 0x01;
        uiB = 0x55;
      }
      else if (time.sunrise > 50)
      {
        uiR = 0xff;
        long x = (time.sunrise - 50);
        uiG = 0x33 + ((0xcc * (x)) / 50);
        uiB = 0x55 + ((0x2b * (x)) / 50);
      }
    }

    uiCount = 0;
  }
  return 0;
}
