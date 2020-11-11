/**
 * @file font_5x8.h
 *
 */
/* Copyright (C) 2020 by Arjan van Vught mailto:info@orangepi-dmx.nl
 * Copyright (C) 2020 by hippy mailto:dmxout@gmail.com
 * 
 * Hippy adapted font from license-free https://github.com/elmo2k3/mLedMatrix/blob/master/mLedMatrix/fonts/5x8.h
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

#ifndef FONT_5x8_H_
#define FONT_5x8_H_

#include <stdint.h>

#define FONT_5x8_CHAR_W	5
#define FONT_5x8_CHAR_H	8

extern uint8_t font_5x8[][8];

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t font_5x8_size(void);

#ifdef __cplusplus
}
#endif



#endif /* FONT_5x8_H_ */
