/**
 * @file h3_hs_timer_init.cpp
 *
 */
/* Copyright (C) 2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#include "h3.h"
#include "h3_ccu.h"

#define CTRL_START		(1U << 0)
#define CTRL_RELOAD		(1U << 1)

void __attribute__((cold)) h3_hs_timer_init() {
	H3_CCU->BUS_CLK_GATING0 |= CCU_BUS_CLK_GATING0_HSTMR;
	H3_CCU->BUS_SOFT_RESET0 |= CCU_BUS_SOFT_RESET0_HSTMR;

	H3_HS_TIMER->IRQ_EN = 0;	// Disable interrupts
	H3_HS_TIMER->CTRL = 0; 		// Timer Stop/Pause

	H3_HS_TIMER->INTV_LO = 0xFFFFFFFF;
	H3_HS_TIMER->INTV_HI = 0x00FFFFFF;

	H3_HS_TIMER->CTRL |= (0x1 << 4);
	H3_HS_TIMER->CTRL |= CTRL_RELOAD;
	H3_HS_TIMER->CTRL |= CTRL_START;
}
