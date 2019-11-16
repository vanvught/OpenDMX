/**
 * @file midireader.h
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

#ifndef H3_MIDIREADER_H_
#define H3_MIDIREADER_H_

#include "ltc.h"
#include "midi.h"

class MidiReader {
public:
	MidiReader (struct TLtcDisabledOutputs *pLtcDisabledOutputs);
	~MidiReader(void);

	void Start(void);
	void Run(void);

private:
	void HandleMtc(void);
	void HandleMtcQf(void);
	void Update(void);

private:
	alignas(uint32_t) struct TLtcDisabledOutputs *m_ptLtcDisabledOutputs;
	alignas(uint32_t) struct _midi_send_tc m_MidiTimeCode;
	_midi_timecode_type m_nTimeCodeType;
	alignas(uint32_t) char m_aTimeCode[TC_CODE_MAX_LENGTH];
	uint8_t m_nPartPrevious;
	bool m_bDirection;
};

#endif /* H3_MIDIREADER_H_ */
