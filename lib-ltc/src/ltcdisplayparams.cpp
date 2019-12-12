/**
 * @file ltcdisplayparams.cpp
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

#if !defined(__clang__)	// Needed for compiling on MacOS
 #pragma GCC push_options
 #pragma GCC optimize ("Os")
#endif

#include <stdint.h>
#include <string.h>
#ifndef NDEBUG
 #include <stdio.h>
#endif
#include <assert.h>

#include "ltcdisplayparams.h"

#include "ltcdisplayparamsconst.h"
#include "devicesparamsconst.h"
// Displays
#include "displaymax7219.h"
#include "displayws28xx.h"
#include "ws28xx.h"
#include "ws28xxconst.h"
#include "rgbmapping.h"

#include "readconfigfile.h"
#include "sscan.h"

static const char aColonBlinkMode[3][5] = { "off", "down", "up" };

LtcDisplayParams::LtcDisplayParams(LtcDisplayParamsStore *pLtcDisplayParamsStore): m_pLtcDisplayParamsStore(pLtcDisplayParamsStore) {
	m_tLtcDisplayParams.nLedType = WS28XXDISPLAY_DEFAULT_LED_TYPE;
	m_tLtcDisplayParams.nGlobalBrightness = WS28XXDISPLAY_DEFAULT_GLOBAL_BRIGHTNESS;
	m_tLtcDisplayParams.nMax7219Type = MAX7219_TYPE_MATRIX;
	m_tLtcDisplayParams.nMax7219Intensity = 4;
	m_tLtcDisplayParams.nRgbMapping = RGB_MAPPING_RGB;
	m_tLtcDisplayParams.nWS28xxIntensity = WS28XXDISPLAY_DEFAULT_MASTER;
	m_tLtcDisplayParams.nWS28xxColonBlinkMode = WS28XXDISPLAY_DEFAULT_COLON_BLINK_MODE;
	m_tLtcDisplayParams.aWS28xxColour[WS28XX_COLOUR_INDEX_SEGMENT] = WS28XXDISPLAY_DEFAULT_COLOUR_SEGMENT;
	m_tLtcDisplayParams.aWS28xxColour[WS28XX_COLOUR_INDEX_COLON] = WS28XXDISPLAY_DEFAULT_COLOUR_COLON;
	m_tLtcDisplayParams.aWS28xxColour[WS28XX_COLOUR_INDEX_MESSAGE] = WS28XXDISPLAY_DEFAULT_COLOUR_MESSAGE;
}

LtcDisplayParams::~LtcDisplayParams(void) {
}

bool LtcDisplayParams::Load(void) {
	m_tLtcDisplayParams.nSetList = 0;

	ReadConfigFile configfile(LtcDisplayParams::staticCallbackFunction, this);

	if (configfile.Read(LtcDisplayParamsConst::FILE_NAME)) {
		// There is a configuration file
		if (m_pLtcDisplayParamsStore != 0) {
			m_pLtcDisplayParamsStore->Update(&m_tLtcDisplayParams);
		}
	} else if (m_pLtcDisplayParamsStore != 0) {
		m_pLtcDisplayParamsStore->Copy(&m_tLtcDisplayParams);
	} else {
		return false;
	}

	return true;
}

void LtcDisplayParams::Load(const char *pBuffer, uint32_t nLength) {
	assert(pBuffer != 0);
	assert(nLength != 0);
	assert(m_pLtcDisplayParamsStore != 0);

	if (m_pLtcDisplayParamsStore == 0) {
		return;
	}

	m_tLtcDisplayParams.nSetList = 0;

	ReadConfigFile config(LtcDisplayParams::staticCallbackFunction, this);

	config.Read(pBuffer, nLength);

	m_pLtcDisplayParamsStore->Update(&m_tLtcDisplayParams);
}

void LtcDisplayParams::callbackFunction(const char *pLine) {
	char buffer[16];
	uint8_t value8;
	uint32_t value32;
	uint8_t len;

	len = sizeof(buffer);

	if (Sscan::Char(pLine, LtcDisplayParamsConst::MAX7219_TYPE, buffer, &len) == SSCAN_OK) {
		if (strncasecmp(buffer, "7segment", len) == 0) {
			m_tLtcDisplayParams.nMax7219Type = MAX7219_TYPE_7SEGMENT;
			m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_MAX7219_TYPE;
		} else if (strncasecmp(buffer, "matrix", len) == 0) {
			m_tLtcDisplayParams.nMax7219Type = MAX7219_TYPE_MATRIX;
			m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_MAX7219_TYPE;
		}
	}

	if (Sscan::Uint8(pLine, LtcDisplayParamsConst::MAX7219_INTENSITY, &value8) == SSCAN_OK) {
		if (value8 <= 0x0F) {
			m_tLtcDisplayParams.nMax7219Intensity = value8;
			m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_MAX7219_INTENSITY;
		}
		return;
	}

	len = 7;
	if (Sscan::Char(pLine, DevicesParamsConst::LED_TYPE, buffer, &len) == SSCAN_OK) {
		buffer[len] = '\0';
		for (uint32_t i = 0; i < WS28XX_UNDEFINED; i++) {
			if (strcasecmp(buffer, WS28xxConst::TYPES[i]) == 0) {
				m_tLtcDisplayParams.nLedType = (TWS28XXType) i;
				m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_LED_TYPE;
				return;
			}
		}
		return;
	}

	len = 3;
	if (Sscan::Char(pLine, DevicesParamsConst::LED_RGB_MAPPING, buffer, &len) == SSCAN_OK) {
		buffer[len] = '\0';
		enum TRGBMapping tMapping;
		if ((tMapping = RGBMapping::FromString(buffer)) != RGB_MAPPING_UNDEFINED) {
			m_tLtcDisplayParams.nRgbMapping = (uint8_t) tMapping;
			m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_RGB_MAPPING;
		}
		return;
	}

	if (Sscan::Uint8(pLine, LtcDisplayParamsConst::WS28XX_INTENSITY, &value8) == SSCAN_OK) {
		if (value8 != 0) {
			m_tLtcDisplayParams.nWS28xxIntensity = value8;
			m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_WS28XX_INTENSITY;
		}
		return;
	}

	len = 4;
	if (Sscan::Char(pLine, LtcDisplayParamsConst::WS28XX_COLON_BLINK_MODE, buffer, &len) == SSCAN_OK) {
		buffer[len] = '\0';
		for (uint32_t i = 0; i < (sizeof(aColonBlinkMode) / sizeof(aColonBlinkMode[0])); i++) {
			if (strcasecmp(buffer, aColonBlinkMode[i]) == 0) {
				m_tLtcDisplayParams.nWS28xxColonBlinkMode = (TColonBlinkMode) i;
				m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_WS28XX_COLON_BLINK_MODE;
				return;
			}
		}
		return;
	}

	for (uint32_t nIndex = 0; nIndex < WS28XX_COLOUR_INDEX_LAST; nIndex++) {
		if(Sscan::Hex24Uint32(pLine, LtcDisplayParamsConst::WS28XX_COLOUR[nIndex], &value32) == SSCAN_OK) {
			m_tLtcDisplayParams.aWS28xxColour[nIndex] = value32;
			m_tLtcDisplayParams.nSetList |= (LTCDISPLAY_PARAMS_MASK_WS28XX_COLOUR_INDEX << nIndex);
			return;
		}
	}

	if (Sscan::Uint8(pLine, DevicesParamsConst::GLOBAL_BRIGHTNESS, &value8) == SSCAN_OK) {
		m_tLtcDisplayParams.nGlobalBrightness = value8;
		m_tLtcDisplayParams.nSetList |= LTCDISPLAY_PARAMS_MASK_GLOBAL_BRIGHTNESS;
		return;
	}
}

void LtcDisplayParams::Set(DisplayWS28xx *pDisplayWS28xx) {
	assert(pDisplayWS28xx != 0);

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_RGB_MAPPING)) {
		pDisplayWS28xx->SetMapping((TRGBMapping) m_tLtcDisplayParams.nRgbMapping);
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_WS28XX_INTENSITY)) {
		pDisplayWS28xx->SetMaster(m_tLtcDisplayParams.nWS28xxIntensity);
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_WS28XX_COLON_BLINK_MODE)) {
		pDisplayWS28xx->SetColonBlinkMode((TColonBlinkMode) m_tLtcDisplayParams.nWS28xxColonBlinkMode);
	}

	for (uint32_t nIndex = 0; nIndex < WS28XX_COLOUR_INDEX_LAST; nIndex++) {
		if (isMaskSet((LTCDISPLAY_PARAMS_MASK_WS28XX_COLOUR_INDEX << nIndex))) {
			pDisplayWS28xx->SetColour(m_tLtcDisplayParams.aWS28xxColour[nIndex], (TWS28xxColourIndex) nIndex);
		}
	}
}

void LtcDisplayParams::Dump(void) {
#ifndef NDEBUG
	if (m_tLtcDisplayParams.nSetList == 0) {
		return;
	}

	printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, LtcDisplayParamsConst::FILE_NAME);

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_LED_TYPE)) {
		printf(" %s=%s [%d]\n", DevicesParamsConst::LED_TYPE, WS28xx::GetLedTypeString((TWS28XXType) m_tLtcDisplayParams.nLedType), (int) m_tLtcDisplayParams.nLedType);
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_RGB_MAPPING)) {
		printf(" %s=%s [%d]\n", DevicesParamsConst::LED_RGB_MAPPING, RGBMapping::ToString((TRGBMapping) m_tLtcDisplayParams.nRgbMapping), (int) m_tLtcDisplayParams.nRgbMapping);
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_WS28XX_INTENSITY)) {
		printf(" %s=%d\n", LtcDisplayParamsConst::WS28XX_INTENSITY, m_tLtcDisplayParams.nWS28xxIntensity);
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_WS28XX_COLON_BLINK_MODE)) {
		printf(" %s=%d\n", LtcDisplayParamsConst::WS28XX_COLON_BLINK_MODE, m_tLtcDisplayParams.nWS28xxColonBlinkMode);
	}

	for (uint32_t nIndex = 0; nIndex < WS28XX_COLOUR_INDEX_LAST; nIndex++) {
		if (isMaskSet((LTCDISPLAY_PARAMS_MASK_WS28XX_COLOUR_INDEX << nIndex))) {
			printf(" %s=%.6x\n", LtcDisplayParamsConst::WS28XX_COLOUR[nIndex], m_tLtcDisplayParams.aWS28xxColour[nIndex]);
		}
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_GLOBAL_BRIGHTNESS)) {
		printf(" %s=%d\n", DevicesParamsConst::GLOBAL_BRIGHTNESS, (int) m_tLtcDisplayParams.nGlobalBrightness);
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_MAX7219_TYPE)) {
		printf(" %s=%d [%s]\n", LtcDisplayParamsConst::MAX7219_TYPE, m_tLtcDisplayParams.nMax7219Type, m_tLtcDisplayParams.nMax7219Type == MAX7219_TYPE_7SEGMENT ? "7segment" : "matrix");
	}

	if (isMaskSet(LTCDISPLAY_PARAMS_MASK_MAX7219_INTENSITY)) {
		printf(" %s=%d\n", LtcDisplayParamsConst::MAX7219_INTENSITY, m_tLtcDisplayParams.nMax7219Intensity);
	}
#endif
}

void LtcDisplayParams::staticCallbackFunction(void *p, const char *s) {
	assert(p != 0);
	assert(s != 0);

	((LtcDisplayParams *) p)->callbackFunction(s);
}

bool LtcDisplayParams::isMaskSet(uint32_t nMask) const {
	return (m_tLtcDisplayParams.nSetList & nMask) == nMask;
}
