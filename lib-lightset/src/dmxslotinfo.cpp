/**
 * @file dmxslotinfo.h
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
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "dmxslotinfo.h"

#include "lightset.h"

#include "debug.h"

#define TO_HEX(i)	((i) < 10) ? '0' + (i) : 'A' + ((i) - 10)

DmxSlotInfo::DmxSlotInfo(struct TLightSetSlotInfo *ptLightSetSlotInfo, uint32_t nSize):
	m_ptLightSetSlotInfo(ptLightSetSlotInfo),
	m_nSize(nSize),
	m_pToString(0)
{
	DEBUG_ENTRY

	assert(m_ptLightSetSlotInfo != 0);
	assert(m_nSize != 0);

	for (uint32_t i = 0; i < m_nSize; i++) {
		m_ptLightSetSlotInfo[i].nType = 0x00;		// ST_PRIMARY
		m_ptLightSetSlotInfo[i].nCategory = 0xFFFF;	// SD_UNDEFINED
	}

	DEBUG_EXIT
}

DmxSlotInfo::~DmxSlotInfo(void) {
	DEBUG_ENTRY

	if (m_pToString != 0) {
		delete[] m_pToString;
		m_pToString = 0;
	}

	DEBUG_EXIT
}

void  DmxSlotInfo::FromString(const char *pString, uint32_t &nMask) {
	assert(pString != 0);

	char *pSlotInfoRaw = (char *)pString;
	nMask = 0;


	for (uint32_t i = 0; i < m_nSize; i++) {
		bool isSet = false;
		struct TLightSetSlotInfo tLightSetSlotInfo;

		if (pSlotInfoRaw == 0) {
			break;
		}

		pSlotInfoRaw = Parse(pSlotInfoRaw, isSet, tLightSetSlotInfo);

		if (isSet) {
			m_ptLightSetSlotInfo[i].nType = tLightSetSlotInfo.nType;
			m_ptLightSetSlotInfo[i].nCategory = tLightSetSlotInfo.nCategory;
			nMask |= (1 << i);
		}
	}

	DEBUG_PRINTF("nMask=0x%x", (int) nMask);
}

const char *DmxSlotInfo::ToString(uint32_t nMask) {
	if (m_pToString == 0) {
		m_pToString = new char[m_nSize * 7];
		assert(m_pToString != 0);

		m_pToString[0] = '\0';
	}

	DEBUG_PRINTF("nMask=0x%x", nMask);

	if (nMask == 0) {
		m_pToString[0] = '\0';
		return (const char *)m_pToString;
	}

	char *p = m_pToString;

	for (uint32_t i = 0; i < m_nSize; i++) {
		if ((nMask & 0x1) == 0x1) {
			const uint8_t nType = m_ptLightSetSlotInfo[i].nType;
			const uint16_t nCategory = m_ptLightSetSlotInfo[i].nCategory;

			*p++ = TO_HEX((nType & 0xF0) >> 4);
			*p++ = TO_HEX(nType & 0x0F);
			*p++ = ':';
			*p++ = TO_HEX((nCategory & 0xF000) >> 12);
			*p++ = TO_HEX((nCategory & 0x0F00) >> 8);
			*p++ = TO_HEX((nCategory & 0x00F0) >> 4);
			*p++ = TO_HEX(nCategory & 0x000F);
			*p++ = ',';
		}

		nMask = nMask >> 1;
	}

	p--;
	*p = '\0';

	DEBUG_PRINTF("%u %u", (uint32_t)(p - m_pToString), (uint32_t)(m_nSize * 7));

	assert((uint32_t)(p - m_pToString) <= (uint32_t)(m_nSize * 7));

	return (const char *)m_pToString;
}

void DmxSlotInfo::Dump(void) {
	for (uint32_t i = 0; i < m_nSize; i++) {
		printf("  Slot:%d %.2X:%.4X\n", i, m_ptLightSetSlotInfo[i].nType, m_ptLightSetSlotInfo[i].nCategory);
	}
}

char *DmxSlotInfo::Parse(char *s, bool &isValid, struct TLightSetSlotInfo &tLightSetSlotInfo) {
	assert(s != 0);

	char *b = s;
	uint8_t i = 0;

	uint16_t nTmp = 0;

	while ((i < 2) && (*b != ':')) {
		if (isxdigit((int)*b) == 0) {
			isValid = false;
			return 0;
		}

		const uint8_t nibble = *b > '9' ? (*b | 0x20) - 'a' + 10 : (*b - '0');
		nTmp = (nTmp << 4) | nibble;
		b++;
		i++;
	}

	if ((i != 2) && (*b != ':')) {
		isValid = false;
		return 0;
	}

	tLightSetSlotInfo.nType = nTmp;

	i = 0;
	nTmp = 0;

	b++;

	while ((i < 4) && (*b != ',') && (*b != '\0')) {
		if (isxdigit((int) *b) == 0) {
			isValid = false;
			return 0;
		}

		const uint8_t nibble = *b > '9' ? (*b | 0x20) - 'a' + 10 : (*b - '0');
		nTmp = (nTmp << 4) | nibble;
		b++;
		i++;
	}

	if (i != 4) {
		isValid = false;
		return 0;
	}

	if ((*b != ',') && (*b != ' ') && (*b != '\0')) {
		isValid = false;
		return 0;
	}

	tLightSetSlotInfo.nCategory = nTmp;

	isValid = true;

	if (*b == '\0') {
		return 0;
	}

	return ++b;
}
