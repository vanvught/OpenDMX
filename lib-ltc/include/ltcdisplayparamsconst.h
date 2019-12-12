/**
 * @file ltcdisplayparamsconst.h
 *
 */
/* Copyright (C) 2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#ifndef LTCDISPLAYPARAMSCONST_H_
#define LTCDISPLAYPARAMSCONST_H_

#include <stdint.h>

#include "displayws28xx.h"

class LtcDisplayParamsConst {
public:
	alignas(uint32_t) static const char FILE_NAME[];

	alignas(uint32_t) static const char MAX7219_TYPE[];
	alignas(uint32_t) static const char MAX7219_INTENSITY[];

	alignas(uint32_t) static const char WS28XX_INTENSITY[];
	alignas(uint32_t) static const char WS28XX_COLON_BLINK_MODE[];
	alignas(uint32_t) static const char WS28XX_COLOUR[WS28XX_COLOUR_INDEX_LAST][24];
};

#endif /* LTCDISPLAYPARAMSCONST_H_ */
