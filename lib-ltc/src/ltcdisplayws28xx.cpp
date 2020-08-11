/**
 * @file ltcdisplayws28xx.cpp
 */
/*
 * Copyright (C) 2019-2020 by hippy mailto:dmxout@gmail.com
 * Copyright (C) 2019-2020 by Arjan van Vught mailto:info@orangepi-dmx.nl
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
#include <stdio.h>
#include <cassert>

#include "ltcdisplayws28xx.h"

#include "ltcdisplayws28xx7segment.h"
#include "ltcdisplayws28xxmatrix.h"

#include "ltc.h"

#include "rgbmapping.h"

#include "hardware.h"
#include "network.h"

#include "debug.h"

namespace RGB {
	static constexpr char PATH[] = "rgb";
	static constexpr auto LENGTH = sizeof(PATH) - 1;
	static constexpr auto HEX_SIZE = 7; // 1 byte index followed by 6 bytes hex RGB
}

namespace MASTER {
	static constexpr char PATH[] = "master";
	static constexpr auto LENGTH = sizeof(PATH) - 1;
	static constexpr auto HEX_SIZE = 2;
}

namespace SHOWMSG {
	static constexpr char PATH[] = "showmsg";
	static constexpr auto LENGTH = sizeof(PATH) - 1;
}

namespace udp {
	static constexpr auto PORT = 0x2812;
}

#define MESSAGE_TIME_MS		3000

LtcDisplayWS28xx *LtcDisplayWS28xx::s_pThis = nullptr;

LtcDisplayWS28xx::LtcDisplayWS28xx(TLtcDisplayWS28xxTypes tType) :
	m_tDisplayWS28xxTypes(tType),
	m_nIntensity(LTCDISPLAYWS28XX_DEFAULT_GLOBAL_BRIGHTNESS),
	m_nHandle(-1),
	m_tMapping(RGB_MAPPING_UNDEFINED),
	m_tLedType(WS28XX_UNDEFINED),
	m_nMaster(LTCDISPLAYWS28XX_DEFAULT_MASTER),
	m_bShowMsg(false),
	m_nMsgTimer(0),
	m_nColonBlinkMillis(0),
	m_nSecondsPrevious(60),
	m_tColonBlinkMode(LTCDISPLAYWS28XX_COLON_BLINK_MODE_DOWN),
	m_pLtcDisplayWS28xxSet(nullptr)
{
	DEBUG_ENTRY

	assert(s_pThis == nullptr);
	s_pThis = this;

	m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_DIGIT] = LTCDISPLAYWS28XX_DEFAULT_COLOUR_DIGIT;
	m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_COLON] = LTCDISPLAYWS28XX_DEFAULT_COLOUR_COLON;
	m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_MESSAGE] = LTCDISPLAYWS28XX_DEFAULT_COLOUR_MESSAGE;

	DEBUG_EXIT
}

LtcDisplayWS28xx::~LtcDisplayWS28xx() {
	DEBUG_ENTRY

	assert(m_pLtcDisplayWS28xxSet == nullptr);

	delete m_pLtcDisplayWS28xxSet;
	m_pLtcDisplayWS28xxSet = nullptr;

	DEBUG_EXIT
}

void LtcDisplayWS28xx::Init(TWS28XXType tLedType, __attribute__((unused)) uint8_t nIntensity) {
	DEBUG_ENTRY

	m_tLedType = tLedType;

	if (m_tDisplayWS28xxTypes == LTCDISPLAYWS28XX_TYPE_7SEGMENT) {
		m_pLtcDisplayWS28xxSet = new LtcDisplayWS28xx7Segment;
	} else {
		m_pLtcDisplayWS28xxSet = new LtcDisplayWS28xxMatrix;
	}

	assert(m_pLtcDisplayWS28xxSet != nullptr);
	m_pLtcDisplayWS28xxSet->Init(tLedType, m_tMapping);

	SetRGB(m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_DIGIT], LTCDISPLAYWS28XX_COLOUR_INDEX_DIGIT);
	SetRGB(m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_COLON], LTCDISPLAYWS28XX_COLOUR_INDEX_COLON);
	SetRGB(m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_MESSAGE], LTCDISPLAYWS28XX_COLOUR_INDEX_MESSAGE);

	m_nHandle = Network::Get()->Begin(udp::PORT);
	assert(m_nHandle != -1);

	DEBUG_EXIT
}

void LtcDisplayWS28xx::Show(const char *pTimecode) {
	if (__builtin_expect((m_bShowMsg), 0)) {
		ShowMessage();
		return;
	}

	struct TLtcDisplayRgbColours tColoursColons;

	if (m_tColonBlinkMode != LTCDISPLAYWS28XX_COLON_BLINK_MODE_OFF) {
		const uint32_t nMillis = Hardware::Get()->Millis();

		if (m_nSecondsPrevious != pTimecode[LTC_TC_INDEX_SECONDS_UNITS]) { // seconds have changed
			m_nSecondsPrevious = pTimecode[LTC_TC_INDEX_SECONDS_UNITS];
			m_nColonBlinkMillis = nMillis;

			tColoursColons.nRed = 0;
			tColoursColons.nGreen = 0;
			tColoursColons.nBlue = 0;
		} else if (nMillis - m_nColonBlinkMillis < 1000) {
			uint32_t nMaster;

			if (m_tColonBlinkMode == LTCDISPLAYWS28XX_COLON_BLINK_MODE_DOWN) {
				nMaster = 255 - ((nMillis - m_nColonBlinkMillis) * 255 / 1000);
			} else {
				nMaster = ((nMillis - m_nColonBlinkMillis) * 255 / 1000);
			}

			if (!(m_nMaster == 0 || m_nMaster == 255)) {
				nMaster = (m_nMaster * nMaster) / 255 ;
			}

			tColoursColons.nRed = (nMaster * m_tColoursColons.nRed) / 255;
			tColoursColons.nGreen = (nMaster *  m_tColoursColons.nGreen) / 255;
			tColoursColons.nBlue = (nMaster * m_tColoursColons.nBlue) / 255;
		} else {
			if (!(m_nMaster == 0 || m_nMaster == 255)) {
				tColoursColons.nRed = (m_nMaster * m_tColoursColons.nRed) / 255 ;
				tColoursColons.nGreen = (m_nMaster * m_tColoursColons.nGreen) / 255 ;
				tColoursColons.nBlue = (m_nMaster * m_tColoursColons.nBlue) / 255 ;
			} else {
				tColoursColons.nRed = m_tColoursColons.nRed;
				tColoursColons.nGreen = m_tColoursColons.nGreen;
				tColoursColons.nBlue = m_tColoursColons.nBlue;
			}
		}
	} else {
		if (!(m_nMaster == 0 || m_nMaster == 255)) {
			tColoursColons.nRed = (m_nMaster * m_tColoursColons.nRed) / 255 ;
			tColoursColons.nGreen = (m_nMaster * m_tColoursColons.nGreen) / 255 ;
			tColoursColons.nBlue = (m_nMaster * m_tColoursColons.nBlue) / 255 ;
		} else {
			tColoursColons.nRed = m_tColoursColons.nRed;
			tColoursColons.nGreen = m_tColoursColons.nGreen;
			tColoursColons.nBlue = m_tColoursColons.nBlue;
		}
	}

	struct TLtcDisplayRgbColours tColours;

	if (!(m_nMaster == 0 || m_nMaster == 255)) {
		tColours.nRed = (m_nMaster * m_tColours.nRed) / 255 ;
		tColours.nGreen = (m_nMaster * m_tColours.nGreen) / 255 ;
		tColours.nBlue = (m_nMaster * m_tColours.nBlue) / 255 ;
	} else {
		tColours.nRed = m_tColours.nRed;
		tColours.nGreen = m_tColours.nGreen;
		tColours.nBlue = m_tColours.nBlue;
	}

	m_pLtcDisplayWS28xxSet->Show(pTimecode, tColours, tColoursColons);
}

void LtcDisplayWS28xx::ShowSysTime(const char *pSystemTime) {
	if (__builtin_expect((m_bShowMsg), 0)) {
		ShowMessage();
		return;
	}

	struct TLtcDisplayRgbColours tColours;
	struct TLtcDisplayRgbColours tColoursColons;

	if (!(m_nMaster == 0 || m_nMaster == 255)) {
		tColours.nRed = (m_nMaster * m_tColours.nRed) / 255 ;
		tColours.nGreen = (m_nMaster * m_tColours.nGreen) / 255 ;
		tColours.nBlue = (m_nMaster * m_tColours.nBlue) / 255 ;
		//
		tColoursColons.nRed = (m_nMaster * m_tColoursColons.nRed) / 255 ;
		tColoursColons.nGreen = (m_nMaster * m_tColoursColons.nGreen) / 255 ;
		tColoursColons.nBlue = (m_nMaster * m_tColoursColons.nBlue) / 255 ;
	} else {
		tColours.nRed = m_tColours.nRed;
		tColours.nGreen = m_tColours.nGreen;
		tColours.nBlue = m_tColours.nBlue;
		//
		tColoursColons.nRed = m_tColoursColons.nRed;
		tColoursColons.nGreen = m_tColoursColons.nGreen;
		tColoursColons.nBlue = m_tColoursColons.nBlue;
	}

	m_pLtcDisplayWS28xxSet->ShowSysTime(pSystemTime, tColours, tColoursColons);
}

void LtcDisplayWS28xx::SetMessage(const char *pMessage, uint32_t nSize) {
	assert(pMessage != nullptr);

	uint32_t i;
	const char *pSrc = pMessage;
	char *pDst = m_aMessage;

	for (i = 0; i < nSize; i++) {
		*pDst++ = *pSrc++;
	}

	for (; i < sizeof(m_aMessage); i++) {
		*pDst++ = ' ';
	}

	m_nMsgTimer = Hardware::Get()->Millis();
	m_bShowMsg = true;
}

void LtcDisplayWS28xx::ShowMessage() {
	struct TLtcDisplayRgbColours tColours;

	const uint32_t nMillis = Hardware::Get()->Millis();

	tColours.nRed = ((nMillis - m_nMsgTimer) * m_tColoursMessage.nRed) / MESSAGE_TIME_MS;
	tColours.nGreen = ((nMillis - m_nMsgTimer) * m_tColoursMessage.nGreen) / MESSAGE_TIME_MS;
	tColours.nBlue = ((nMillis - m_nMsgTimer) * m_tColoursMessage.nBlue) / MESSAGE_TIME_MS;

	m_pLtcDisplayWS28xxSet->ShowMessage(m_aMessage, tColours);
}

void LtcDisplayWS28xx::WriteChar(uint8_t nChar, uint8_t nPos) {
	struct TLtcDisplayRgbColours tColours;

	if (!(m_nMaster == 0 || m_nMaster == 255)) {
		tColours.nRed = (m_nMaster * m_tColours.nRed) / 255 ;
		tColours.nGreen = (m_nMaster * m_tColours.nGreen) / 255 ;
		tColours.nBlue = (m_nMaster * m_tColours.nBlue) / 255 ;
	} else {
		tColours.nRed = m_tColours.nRed;
		tColours.nGreen = m_tColours.nGreen;
		tColours.nBlue = m_tColours.nBlue;
	}

	m_pLtcDisplayWS28xxSet->WriteChar(nChar, nPos, tColours);
}

void LtcDisplayWS28xx::Run() {
	if (__builtin_expect((m_bShowMsg), 0)) {
		if (Hardware::Get()->Millis() - m_nMsgTimer >= MESSAGE_TIME_MS) {
			m_bShowMsg = false;
		}
	}

	uint32_t nIPAddressFrom;
	uint16_t nForeignPort;
	uint16_t m_nBytesReceived = Network::Get()->RecvFrom(m_nHandle, &m_Buffer, sizeof(m_Buffer), &nIPAddressFrom, &nForeignPort);

	if (__builtin_expect((m_nBytesReceived < 8), 1)) {
		return;
	}

	if (__builtin_expect((memcmp("7seg!", m_Buffer, 5) != 0), 0)) {
		return;
	}

	if (m_Buffer[m_nBytesReceived - 1] == '\n') {
		DEBUG_PUTS("\'\\n\'");
		m_nBytesReceived--;
	}

	if (memcmp(&m_Buffer[5], SHOWMSG::PATH, SHOWMSG::LENGTH) == 0) {
		const uint32_t nMsgLength = m_nBytesReceived - (5 + SHOWMSG::LENGTH + 1);
		DEBUG_PRINTF("m_nBytesReceived=%d, nMsgLength=%d [%.*s]", m_nBytesReceived, nMsgLength, nMsgLength, &m_Buffer[(5 + SHOWMSG::LENGTH + 1)]);

		if (((nMsgLength > 0) && (nMsgLength <= LTCDISPLAY_MAX_MESSAGE_SIZE)) && (m_Buffer[5 + SHOWMSG::LENGTH] == '#')) {
			SetMessage(&m_Buffer[(5 + SHOWMSG::LENGTH + 1)], nMsgLength);
			return;
		}

		DEBUG_PUTS("Invalid !showmsg command");
		return;
	}

	if (memcmp(&m_Buffer[5], RGB::PATH, RGB::LENGTH) == 0) {
		if ((m_nBytesReceived == (5 + RGB::LENGTH + 1 + RGB::HEX_SIZE)) && (m_Buffer[5 + RGB::LENGTH] == '#')) {
			SetRGB(&m_Buffer[(5 + RGB::LENGTH + 1)]);
			return;
		}

		DEBUG_PUTS("Invalid !rgb command");
		return;
	}

	if (memcmp(&m_Buffer[5], MASTER::PATH, MASTER::LENGTH) == 0) {
		if ((m_nBytesReceived == (5 + MASTER::LENGTH + 1 + MASTER::HEX_SIZE)) && (m_Buffer[5 + MASTER::LENGTH] == '#')) {
			m_nMaster = hexadecimalToDecimal(&m_Buffer[(5 + MASTER::LENGTH + 1)], MASTER::HEX_SIZE);
			return;
		}

		DEBUG_PUTS("Invalid !master command");
		return;
	}

	DEBUG_PUTS("Invalid command");
}

void LtcDisplayWS28xx::Print() {
	printf("Display WS28xx\n");
	printf(" Type    : %s [%d]\n", WS28xx::GetLedTypeString(m_tLedType), m_tLedType);
	printf(" Mapping : %s [%d]\n", RGBMapping::ToString(m_tMapping), m_tMapping);
	printf(" Master  : %d\n", m_nMaster);
	printf(" RGB     : Character 0x%.6X, Colon 0x%.6X, Message 0x%.6X\n", m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_DIGIT], m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_COLON], m_aColour[LTCDISPLAYWS28XX_COLOUR_INDEX_MESSAGE]);

	m_pLtcDisplayWS28xxSet->Print();
}
