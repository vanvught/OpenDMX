/**
 * @file ltcoutputs.h
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

#ifndef H3_LTCOUTPUTS_H_
#define H3_LTCOUTPUTS_H_

#include <stdbool.h>

#include "ltc.h"

class LtcOutputs {
public:
	LtcOutputs(const struct TLtcDisabledOutputs *pLtcDisabledOutputs, TLtcReaderSource tSource, bool bShowSysTime);
	~LtcOutputs(void);

	void Init(void);
	void Update(const struct TLtcTimeCode *ptLtcTimeCode);
	void UpdateMidiQuarterFrameMessage(const struct TLtcTimeCode *ptLtcTimeCode);

	void ShowSysTime(void);

	void ResetTimeCodeTypePrevious(void) {
		m_tTimeCodeTypePrevious = TC_TYPE_INVALID;
	}

	void Print(void);

	static LtcOutputs* Get(void) {
		return s_pThis;
	}

private:
	void PrintDisabled(bool IsDisabled, const char *p);

private:
	alignas(uint32_t) struct TLtcDisabledOutputs m_tLtcDisabledOutputs;
	bool m_bShowSysTime;
	TTimecodeTypes m_tTimeCodeTypePrevious;
	uint32_t m_nMidiQuarterFramePiece;
	alignas(uint32_t) char m_aTimeCode[TC_CODE_MAX_LENGTH];
	alignas(uint32_t) char m_aSystemTime[TC_SYSTIME_MAX_LENGTH];
	uint32_t m_nSecondsPrevious;

	static LtcOutputs *s_pThis;
};

#endif /* H3_LTCOUTPUTS_H_ */
