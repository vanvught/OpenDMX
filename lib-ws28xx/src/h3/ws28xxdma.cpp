/**
 * @file ws28xxdma.cpp
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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "h3/ws28xxdma.h"
#include "ws28xx.h"

#include "h3_spi.h"

#include "debug.h"

WS28xxDMA::WS28xxDMA(TWS28XXType Type, uint16_t nLEDCount, uint32_t nClockSpeed): WS28xx(Type, nLEDCount, nClockSpeed) {
}

WS28xxDMA::~WS28xxDMA(void) {
	m_pBlackoutBuffer = 0;
	m_pBuffer = 0;
}

bool WS28xxDMA::Initialize(void) {
	uint32_t nSize;

	m_pBuffer = (uint8_t *)h3_spi_dma_tx_prepare(&nSize);
	assert(m_pBuffer != 0);

	const uint32_t nSizeHalf = nSize / 2;
	assert(m_nBufSize <= nSizeHalf);

	if (m_nBufSize > nSizeHalf) {
		return false;
	}

	m_pBlackoutBuffer = m_pBuffer + (nSizeHalf & ~3);

	if (m_tLEDType == APA102) {
		memset(m_pBuffer, 0, 4);
		for (uint32_t i = 0; i < m_nLEDCount; i++) {
			SetLED(i, 0, 0, 0);
		}
		memset(&m_pBuffer[m_nBufSize - 4], 0xFF, 4);
	} else {
		memset(m_pBuffer, m_tLEDType == WS2801 ? 0 : 0xC0, m_nBufSize);
	}

	memcpy(m_pBlackoutBuffer, m_pBuffer, m_nBufSize);

	DEBUG_PRINTF("nSize=%x, m_pBuffer=%p, m_pBlackoutBuffer=%p", nSize, m_pBuffer, m_pBlackoutBuffer);

	Blackout();

	return true;
}

void WS28xxDMA::Update(void) {
	assert(m_pBuffer != 0);
	assert(!IsUpdating());

	h3_spi_dma_tx_start(m_pBuffer, m_nBufSize);
}

void WS28xxDMA::Blackout(void) {
	assert(m_pBlackoutBuffer != 0);
	assert(!IsUpdating());

	h3_spi_dma_tx_start(m_pBlackoutBuffer, m_nBufSize);
}
