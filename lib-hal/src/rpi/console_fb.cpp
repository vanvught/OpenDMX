#if !defined(CONSOLE_NONE)
/**
 * @file console_fb.cpp
 *
 */
/* Copyright (C) 2016-2025 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#include <cstdarg>
#include <cstdint>

#include "console.h"

#include "device/fb.h"

#include "arm/arm.h"

extern unsigned char FONT[] __attribute__((aligned(4)));

#define FB_CHAR_W	8
#define FB_CHAR_H	16

static uint32_t current_x = 0;
static uint32_t current_y = 0;
static uint32_t saved_x = 0;
static uint32_t saved_y = 0;
static uint16_t cur_fore = CONSOLE_WHITE;
static uint16_t cur_back = CONSOLE_BLACK;
static uint16_t saved_fore = CONSOLE_WHITE;
static uint16_t saved_back = CONSOLE_BLACK;

static uint32_t top_row = 0;

#if defined (ARM_ALLOW_MULTI_CORE)
static volatile int lock = 0;
#endif

void console_init() {
	fb_init();
}

uint32_t console_get_line_width() {
	return FB_WIDTH / FB_CHAR_W;
}

void console_set_top_row(uint32_t row) {
	if (row > FB_HEIGHT / FB_CHAR_H) {
		top_row = 0;
	} else {
		top_row = row;
	}

	current_x = 0;
	current_y = row;
}

inline static void clear_row(uint32_t *address) {
	uint16_t i;
	const uint32_t value = ((uint32_t)cur_back << 16) | (uint32_t)cur_back;

	for (i = 0 ; i < (FB_CHAR_H * FB_WIDTH) / 2 ; i++) {
		*address++ =  value;
	}
}

inline static void newline() {
	uint32_t i;
	uint32_t *address;
	uint32_t *to;
	uint32_t *from;

	current_y++;
	current_x = 0;

	if (current_y == FB_HEIGHT / FB_CHAR_H) {
		if (top_row == 0) {
			/* Pointer to row = 0 */
			to = (uint32_t *) (fb_addr);
			/* Pointer to row = 1 */
			from = to + (FB_CHAR_H * FB_WIDTH) / 2;
			/* Copy block from {row = 1, rows} to {row = 0, rows - 1} */
			i = ((FB_HEIGHT - FB_CHAR_H) * FB_WIDTH) / 2;
		} else {
			to = (uint32_t *) (fb_addr) + ((FB_CHAR_H * FB_WIDTH) * top_row) / 2;
			from = to + (FB_CHAR_H * FB_WIDTH) / 2;
			i = ((FB_HEIGHT - FB_CHAR_H) * FB_WIDTH - ((FB_CHAR_H * FB_WIDTH) * top_row)) / 2;
		}

		memcpy_blk(to, from, i/8);

		/* Clear last row */
		address = (uint32_t *)(fb_addr) + ((FB_HEIGHT - FB_CHAR_H) * FB_WIDTH) / 2;
		clear_row(address);

		current_y--;
	}
}

inline static void draw_pixel(uint32_t x, uint32_t y, uint16_t color) {
	volatile uint16_t *address = (volatile uint16_t *)(fb_addr + (x * FB_BYTES_PER_PIXEL) + (y * FB_WIDTH * FB_BYTES_PER_PIXEL));
	*address = color;
}

inline static void draw_char(int c, uint32_t x, uint32_t y, uint16_t fore, uint16_t back) {
	uint32_t i, j;
	uint8_t line;
	unsigned char *p = FONT + (c * (int) FB_CHAR_H);

	for (i = 0; i < FB_CHAR_H; i++) {
		line = (uint8_t) *p++;
		for (j = x; j < (FB_CHAR_W + x); j++) {
			if ((line & 0x1) != 0) {
				draw_pixel(j, y, fore);
			} else {
				draw_pixel(j, y, back);
			}
			line >>= 1;
		}
		y++;
	}
}

int console_draw_char(int ch, uint16_t x, uint16_t y, uint16_t fore, uint16_t back) {
	draw_char(ch, x * FB_CHAR_W, y * FB_CHAR_H, fore, back);
	return (int)ch;
}

void console_putc(int ch) {
#if defined (ARM_ALLOW_MULTI_CORE)
	while (__sync_lock_test_and_set(&lock, 1) == 1);
#endif

	if (ch == (int)'\n') {
		newline();
	} else if (ch == (int)'\r') {
		current_x = 0;
	} else if (ch == (int)'\t') {
		current_x += 4;
	} else {
		draw_char(ch, current_x * FB_CHAR_W, current_y * FB_CHAR_H, cur_fore, cur_back);
		current_x++;
		if (current_x == FB_WIDTH / FB_CHAR_W) {
			newline();
		}
	}

#if defined (ARM_ALLOW_MULTI_CORE)
	__sync_lock_release(&lock);
#endif
}

void console_puts(const char *s) {
	char c;
	int i = 0;;

	while ((c = *s++) != (char) 0) {
		i++;
		console_putc((int) c);
	}
}

void console_write(const char *s, unsigned int n) {
	char c;

	while (((c = *s++) != (char) 0) && (n-- != 0)) {
		console_putc((int) c);
	}
}

void console_error(const char *s) {
	char c;
	int i = 0;;

	uint16_t fore_current = cur_fore;
	uint16_t back_current = cur_back;

	cur_fore = CONSOLE_RED;
	cur_back = CONSOLE_BLACK;

	console_puts("Error <");

	while ((c = *s++) != (char) 0) {
		i++;
		console_putc((int) c);
	}

	console_puts(">\n");

	cur_fore = fore_current;
	cur_back = back_current;
}

void console_status(uint32_t color, const char *s) {
	char c;

	const uint16_t fore_current = cur_fore;
	const uint16_t back_current = cur_back;

	const uint16_t s_y = current_y;
	const uint16_t s_x = current_x;

	console_clear_line(29);

	cur_fore = color;
	cur_back = CONSOLE_BLACK;

	while ((c = *s++) != (char) 0) {
		console_putc((int) c);
	}

	current_y = s_y;
	current_x = s_x;

	cur_fore = fore_current;
	cur_back = back_current;
}

#define TO_HEX(i)	((i) < 10) ? (uint8_t)'0' + (i) : (uint8_t)'A' + ((i) - (uint8_t)10)

void console_puthex(uint8_t data) {
	console_putc((int) (TO_HEX(((data & 0xF0) >> 4))));
	console_putc((int) (TO_HEX(data & 0x0F)));
}

void console_puthex_fg_bg(uint8_t data, uint16_t fore, uint16_t back) {
	uint16_t fore_current = cur_fore;
	uint16_t back_current = cur_back;

	cur_fore = fore;
	cur_back = back;

	console_putc((int) (TO_HEX(((data & 0xF0) >> 4))));
	console_putc((int) (TO_HEX(data & 0x0F)));

	cur_fore = fore_current;
	cur_back = back_current;
}

void console_putpct_fg_bg(uint8_t data, uint16_t fore, uint16_t back) {
	uint16_t fore_current = cur_fore;
	uint16_t back_current = cur_back;

	cur_fore = fore;
	cur_back = back;

	if (data < 100) {
		console_putc((int) ((char) '0' + (char) (data / 10)));
		console_putc((int) ((char) '0' + (char) (data % 10)));
	} else {
		console_puts("%%");
	}

	cur_fore = fore_current;
	cur_back = back_current;
}

void console_put3dec_fg_bg(uint8_t data, uint16_t fore, uint16_t back) {
	uint16_t fore_current = cur_fore;
	uint16_t back_current = cur_back;

	cur_fore = fore;
	cur_back = back;

	const uint8_t i = data / 100;

	console_putc((int) ((char) '0' + (char) i));

	data -= (i * 100);

	console_putc((int) ((char) '0' + (char) (data / 10)));
	console_putc((int) ((char) '0' + (char) (data % 10)));

	cur_fore = fore_current;
	cur_back = back_current;
}

void console_newline(){
	newline();
}

void console_clear() {
	uint16_t *address = (uint16_t *)(fb_addr);
	uint32_t i;

	for (i = 0; i < (FB_HEIGHT * FB_WIDTH); i++) {
		*address++ = cur_back;
	}

	current_x = 0;
	current_y = 0;
}

void console_set_cursor(uint32_t x, uint32_t y) {
#if defined (ARM_ALLOW_MULTI_CORE)
	while (__sync_lock_test_and_set(&lock, 1) == 1);
#endif

	if (x > FB_WIDTH / FB_CHAR_W)
		current_x = 0;
	else
		current_x = x;

	if (y > FB_HEIGHT / FB_CHAR_H)
		current_y = 0;
	else
		current_y = y;

#if defined (ARM_ALLOW_MULTI_CORE)
	__sync_lock_release(&lock);
#endif
}

void console_save_cursor() {
	saved_y = current_y;
	saved_x = current_x;
	saved_back = cur_back;
	saved_fore = cur_fore;
}

void console_restore_cursor() {
	current_y = saved_y;
	current_x = saved_x;
	cur_back = saved_back;
	cur_fore = saved_fore;
}

void console_save_color() {
	saved_back = cur_back;
	saved_fore = cur_fore;
}

void console_restore_color() {
	cur_back = saved_back;
	cur_fore = saved_fore;
}

void console_set_fg_color(uint32_t fore) {
	cur_fore = fore;
}

void console_set_bg_color(uint32_t back) {
	cur_back = back;
}

void console_set_fg_bg_color(uint16_t fore, uint16_t back) {
	cur_fore = fore;
	cur_back = back;
}

void console_clear_line(uint32_t line) {
	uint32_t *address;

	if (line > FB_HEIGHT / FB_CHAR_H) {
		return;
	} else {
		current_y = line;
	}

	current_x = 0;

	address = (uint32_t *)(fb_addr) + (line * FB_CHAR_H * FB_WIDTH) / 2;
	clear_row(address);
}
#endif
