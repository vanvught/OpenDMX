#if !defined(ORANGE_PI)
# error
#endif
/**
 * @file esp8266.cpp
 *
 */
/* Copyright (C) 2018-2024 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include <cstdint>

#include "h3.h"
#include "h3_gpio.h"
#include "h3_board.h"

/*
 *      Orange Pi Zero		ESP8266
 *
 * 11	PA1		--      ->	GPIO5
 * 13	PA0		<-      --	GPIO4
 *
 * 15	PA3		<- DATA ->	GPI012		CFG0
 * 16	PA19	<- DATA ->	GPI013		CFG2
 * 18	PA18	<- DATA ->	GPI014		CFG2
 * 22	PA2		<- DATA ->	GPI015		CFG0
 */

#define COUT	H3_GPIO_TO_NUMBER(GPIO_EXT_11)
#define CIN		H3_GPIO_TO_NUMBER(GPIO_EXT_13)

#if (H3_GPIO_TO_PORT(GPIO_EXT_13) == H3_GPIO_PORTA)
# define PORT_CIN	H3_PIO_PORTA
#else
# error PORT_CIN not defined
#endif

#define D0		H3_GPIO_TO_NUMBER(GPIO_EXT_15)
#define D1		H3_GPIO_TO_NUMBER(GPIO_EXT_16)
#define D2		H3_GPIO_TO_NUMBER(GPIO_EXT_18)
#define D3		H3_GPIO_TO_NUMBER(GPIO_EXT_22)

typedef union pcast32 {
	uint32_t u32;
	uint8_t u8[4];
} esp8266_pcast32;

static void data_gpio_fsel_output() {
	__ISB();

	uint32_t value = H3_PIO_PORTA->CFG0;
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA2_SELECT_CFG0_SHIFT);
	value |= (GPIO_FSEL_OUTPUT << PA2_SELECT_CFG0_SHIFT);
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA3_SELECT_CFG0_SHIFT);
	value |= (GPIO_FSEL_OUTPUT << PA3_SELECT_CFG0_SHIFT);
	H3_PIO_PORTA->CFG0 = value;

	value = H3_PIO_PORTA->CFG2;
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA18_SELECT_CFG2_SHIFT);
	value |= (GPIO_FSEL_OUTPUT << PA18_SELECT_CFG2_SHIFT);
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA19_SELECT_CFG2_SHIFT);
	value |= (GPIO_FSEL_OUTPUT << PA19_SELECT_CFG2_SHIFT);
	H3_PIO_PORTA->CFG2 = value;

	__DMB();
}

static void data_gpio_fsel_input() {
	__ISB();

	uint32_t value = H3_PIO_PORTA->CFG0;
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA2_SELECT_CFG0_SHIFT);
	value |= (GPIO_FSEL_INPUT << PA2_SELECT_CFG0_SHIFT);
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA3_SELECT_CFG0_SHIFT);
	value |= (GPIO_FSEL_INPUT << PA3_SELECT_CFG0_SHIFT);
	H3_PIO_PORTA->CFG0 = value;

	value = H3_PIO_PORTA->CFG2;
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA18_SELECT_CFG2_SHIFT);
	value |= (GPIO_FSEL_INPUT << PA18_SELECT_CFG2_SHIFT);
	value &= (uint32_t) ~(GPIO_SELECT_MASK << PA19_SELECT_CFG2_SHIFT);
	value |= (GPIO_FSEL_INPUT << PA19_SELECT_CFG2_SHIFT);
	H3_PIO_PORTA->CFG2 = value;

	__DMB();
}

void __attribute__((cold)) esp8266_init() {
	h3_gpio_fsel(GPIO_EXT_13, GPIO_FSEL_INPUT);
	h3_gpio_fsel(GPIO_EXT_11, GPIO_FSEL_OUTPUT);



	h3_gpio_clr(GPIO_EXT_11);
	udelay(1000);

	while (PORT_CIN->DAT & (1 << CIN));
}

void esp8266_write_4bits(const uint8_t data) {
	data_gpio_fsel_output();

	// Put the data on the bus.
	uint32_t out_gpio = H3_PIO_PORTA->DAT & (uint32_t)~( (1 << D0) | (1 << D1) | (1 << D2) | (1 << D3) );
	out_gpio |= (data & 1) ? (1 << D0) : 0;
	out_gpio |= (data & 2) ? (1 << D1) : 0;
	out_gpio |= (data & 4) ? (1 << D2) : 0;
	out_gpio |= (data & 8) ? (1 << D3) : 0;
	H3_PIO_PORTA->DAT = out_gpio;

	// tell that we have data available for read
	h3_gpio_set(GPIO_EXT_11);
	// wait for ack, wait for 0 -> 1
	while (!(H3_PIO_PORTA->DAT & (1 << CIN)));
	// we have 1. now wait for 0
	h3_gpio_clr(GPIO_EXT_11); // we acknowledge, and wait for zero
	while (H3_PIO_PORTA->DAT & (1 << CIN));

	__DMB();
}

inline static void _write_byte(const uint8_t data) {
	// Put the data on the bus.
	uint32_t out_gpio = H3_PIO_PORTA->DAT & (uint32_t)~( (1 << D0) | (1 << D1) | (1 << D2) | (1 << D3) );
	out_gpio |= (data & 1) ? (1 << D0) : 0;
	out_gpio |= (data & 2) ? (1 << D1) : 0;
	out_gpio |= (data & 4) ? (1 << D2) : 0;
	out_gpio |= (data & 8) ? (1 << D3) : 0;
	H3_PIO_PORTA->DAT = out_gpio;

	// tell that we have data available for read
	h3_gpio_set(GPIO_EXT_11);
	// wait for ack, wait for 0 -> 1
	while (!(PORT_CIN->DAT & (1 << CIN)));
	// we have 1. now wait for 0
	h3_gpio_clr(GPIO_EXT_11); // we acknowledge, and wait for zero
	while (PORT_CIN->DAT & (1 << CIN));

	// Put the data on the bus.
	out_gpio = H3_PIO_PORTA->DAT & (uint32_t)~( (1 << D0) | (1 << D1) | (1 << D2) | (1 << D3) );
	out_gpio |= (data & 16) ? (1 << D0) : 0;
	out_gpio |= (data & 32) ? (1 << D1) : 0;
	out_gpio |= (data & 64) ? (1 << D2) : 0;
	out_gpio |= (data & 128) ? (1 << D3) : 0;
	H3_PIO_PORTA->DAT = out_gpio;

	// tell that we have data available for read
	h3_gpio_set(GPIO_EXT_11);
	// wait for ack, wait for 0 -> 1
	while (!(PORT_CIN->DAT & (1 << CIN)));
	// we have 1. now wait for 0
	h3_gpio_clr(GPIO_EXT_11); // we acknowledge, and wait for zero
	while (PORT_CIN->DAT & (1 << CIN));
}

void esp8266_write_byte(const uint8_t byte) {
	data_gpio_fsel_output();
	_write_byte(byte);
	__DMB();
}

void esp8266_write_halfword(const uint16_t half_word) {
	data_gpio_fsel_output();
	_write_byte((uint8_t)(half_word & (uint16_t)0xFF));
	_write_byte((uint8_t)((half_word >> 8) & (uint16_t)0xFF));
	__DMB();
}

void esp8266_write_word(const uint32_t word) {
	esp8266_pcast32 u32 __attribute__((aligned(4)));

	data_gpio_fsel_output();

	u32.u32 = word;

	_write_byte(u32.u8[0]);
	_write_byte(u32.u8[1]);
	_write_byte(u32.u8[2]);
	_write_byte(u32.u8[3]);

	__DMB();
}

void esp8266_write_bytes(const uint8_t *data, const uint32_t len) {
	uint8_t *p = (uint8_t *)data;
	uint8_t d;
	uint16_t i;

	data_gpio_fsel_output();

	for (i = 0; i < len; i++) {
		d = *p;
		_write_byte(d);
		p++;
	}

	__DMB();
}

void esp8266_write_str(const char *data) {
	uint8_t *p = (uint8_t *)data;
	uint8_t d;

	data_gpio_fsel_output();

	while (*p != (char)0) {
		d = *p;
		_write_byte(d);
		p++;
	}

	_write_byte(0);

	__DMB();
}

inline static uint8_t _read_byte() {
	uint8_t data;

	h3_gpio_set(GPIO_EXT_11);
	while (!(PORT_CIN->DAT & (1 << CIN)))
		;

	// Read the data from the data port.
	uint32_t in_gpio = H3_PIO_PORTA->DAT;
	uint8_t data_msb = (uint8_t)(((in_gpio & (1U << D0)) ? 1 : 0)
	 | ((in_gpio & (1U << D1)) ? 2 : 0)
	 | ((in_gpio & (1U << D2)) ? 4 : 0)
	 | ((in_gpio & (1U << D3)) ? 8 : 0));

	h3_gpio_clr(GPIO_EXT_11);
	while (PORT_CIN->DAT & (1 << CIN))
		;

	h3_gpio_set(GPIO_EXT_11);
	while (!(PORT_CIN->DAT & (1 << CIN)))
		;

	in_gpio = H3_PIO_PORTA->DAT;
	uint8_t data_lsb = (uint8_t)(((in_gpio & (1 << D0)) ? 1 : 0)
	 | ((in_gpio & (1 << D1)) ? 2 : 0)
	 | ((in_gpio & (1 << D2)) ? 4 : 0)
	 | ((in_gpio & (1 << D3)) ? 8 : 0));

	data = (uint8_t) (data_msb | ((data_lsb & 0x0F) << 4));

	h3_gpio_clr(GPIO_EXT_11);
	while (PORT_CIN->DAT & (1 << CIN))
		;

	return data;
}

uint8_t esp8266_read_byte() {
	uint8_t data;

	data_gpio_fsel_input();

	data = _read_byte();

	__DMB();

	return data;
}

void esp8266_read_bytes(const uint8_t *data, const uint32_t len){
	uint8_t *p = (uint8_t *)data;
	uint32_t i;

	data_gpio_fsel_input();

	for (i = 0 ; i < len; i++) {
		*p = _read_byte();
		p++;
	}

	__DMB();
}

uint16_t esp8266_read_halfword() {
	uint16_t data;

	data_gpio_fsel_input();

	data = _read_byte();
	data |= (uint16_t) (_read_byte() << 8);

	__DMB();

	return data;
}

uint32_t esp8266_read_word() {
	esp8266_pcast32 u32  __attribute__((aligned(4)));

	data_gpio_fsel_input();

	u32.u8[0] = _read_byte();
	u32.u8[1] = _read_byte();
	u32.u8[2] = _read_byte();
	u32.u8[3] = _read_byte();

	__DMB();

	return u32.u32;
}

void esp8266_read_str(char *s, uint32_t *len) {
	const auto *p = s;
	uint8_t ch;
	auto n = *len;

	data_gpio_fsel_input();

	while ((ch = _read_byte()) != (uint8_t) 0) {
		if (n > (uint16_t) 0) {
			*s++ = (char) ch;
			n--;
		}
	}

	*len = (uint32_t) (s - p);

	while (n > 0) {
		*s++ = '\0';
		--n;
	}

	__DMB();
}

bool esp8266_detect() {
	esp8266_init();

	data_gpio_fsel_output();

	// send a CMD_NOP
	const uint32_t out_gpio = H3_PIO_PORTA->DAT & (uint32_t)~( (1 << D0) | (1 << D1) | (1 << D2) | (1 << D3) );
	H3_PIO_PORTA->DAT = out_gpio;

	h3_gpio_set(GPIO_EXT_11);// Tell that we have data available. Wait for ack, wait for 0 -> 1

	const auto micros_now = H3_TIMER->AVS_CNT1;

	// wait 0.5 second for the ESP8266 to respond
	while ((!(PORT_CIN->DAT & (1 << CIN))) && (H3_TIMER->AVS_CNT1 - micros_now < 500000U))
		;

	if (!(PORT_CIN->DAT & (1 << CIN))) {
		h3_gpio_clr(GPIO_EXT_11);
		return false;
	}

	h3_gpio_clr(GPIO_EXT_11); // we acknowledge, and wait for zero

	while (PORT_CIN->DAT & (1 << CIN));

	__DMB();

	return true;
}
