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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "h3/midireader.h"
#include "ltc.h"

#include "c/led.h"

#include "arm/synchronize.h"
#include "h3_hs_timer.h"
#include "h3_timer.h"
#include "irq_timer.h"

// Input
#include "midi.h"

// Output
#include "ltcleds.h"
#include "artnetnode.h"
#include "rtpmidi.h"
#include "display.h"
#include "displaymax7219.h"
#include "displayws28xx.h"
#include "ntpserver.h"

#ifndef ALIGNED
 #define ALIGNED __attribute__ ((aligned (4)))
#endif

// IRQ Timer0
static volatile uint32_t nUpdatesPerSecond = 0;
static volatile uint32_t nUpdatesPrevious = 0;
static volatile uint32_t nUpdates = 0;

static uint8_t qf[8] ALIGNED = { 0, 0, 0, 0, 0, 0, 0, 0 };	///<

inline static void itoa_base10(uint32_t arg, char *buf) {
	char *n = buf;

	if (arg == 0) {
		*n++ = '0';
		*n = '0';
		return;
	}

	*n++ = (char) ('0' + (arg / 10));
	*n = (char) ('0' + (arg % 10));
}

static void irq_timer0_update_handler(uint32_t clo) {
	dmb();
	nUpdatesPerSecond = nUpdates - nUpdatesPrevious;
	nUpdatesPrevious = nUpdates;
}

MidiReader::MidiReader(struct TLtcDisabledOutputs *pLtcDisabledOutputs):
	m_ptLtcDisabledOutputs(pLtcDisabledOutputs),
	m_nTimeCodeType(MIDI_TC_TYPE_UNKNOWN),
	m_nTimeCodeTypePrevious(MIDI_TC_TYPE_UNKNOWN),
	m_nPartPrevious(0),
	m_bDirection(true)
{
	assert(m_ptLtcDisabledOutputs != 0);

	Ltc::InitTimeCode(m_aTimeCode);
}

MidiReader::~MidiReader(void) {
}

void MidiReader::Start(void) {
	irq_timer_init();

	irq_timer_set(IRQ_TIMER_0, (thunk_irq_timer_t) irq_timer0_update_handler);
	H3_TIMER->TMR0_INTV = 0xB71B00; // 1 second
	H3_TIMER->TMR0_CTRL &= ~(TIMER_CTRL_SINGLE_MODE);
	H3_TIMER->TMR0_CTRL |= (TIMER_CTRL_EN_START | TIMER_CTRL_RELOAD);

	midi_active_set_sense(false); //TODO We do nothing with sense data, yet
	midi_init(MIDI_DIRECTION_INPUT);
}

void MidiReader::Run(void) {
	bool isMtc = false;
	uint8_t nSystemExclusiveLength;
	const uint8_t *pSystemExclusive = Midi::Get()->GetSystemExclusive(nSystemExclusiveLength);

	if (Midi::Get()->Read(MIDI_CHANNEL_OMNI)) {
		if (Midi::Get()->GetChannel() == 0) {
			switch (Midi::Get()->GetMessageType()) {
			case MIDI_TYPES_TIME_CODE_QUARTER_FRAME:
				HandleMtcQf(); // type = midi_reader_mtc_qf(midi_message);
				isMtc = true;
				break;
			case MIDI_TYPES_SYSTEM_EXCLUSIVE:
				if ((pSystemExclusive[1] == 0x7F) && (pSystemExclusive[2] == 0x7F) && (pSystemExclusive[3] == 0x01)) {
					HandleMtc(); // type = midi_reader_mtc(midi_message);
					isMtc = true;
				}
				break;
			default:
				break;
			}
		}
	}

	if (isMtc) {
		nUpdates++;
	}

	dmb();
	if (nUpdatesPerSecond >= 24)  {
		led_set_ticks_per_second(1000000 / 3);
	} else {
		led_set_ticks_per_second(1000000 / 1);
	}
}

void MidiReader::HandleMtc(void) {
	uint8_t nSystemExclusiveLength;
	const uint8_t *pSystemExclusive = Midi::Get()->GetSystemExclusive(nSystemExclusiveLength);

	m_nTimeCodeType = (_midi_timecode_type) (pSystemExclusive[5] >> 5);

	itoa_base10((pSystemExclusive[5] & 0x1F), (char *) &m_aTimeCode[0]);
	itoa_base10(pSystemExclusive[6], (char *) &m_aTimeCode[3]);
	itoa_base10(pSystemExclusive[7], (char *) &m_aTimeCode[6]);
	itoa_base10(pSystemExclusive[8], (char *) &m_aTimeCode[9]);

	m_MidiTimeCode.nHours = pSystemExclusive[5] & 0x1F;
	m_MidiTimeCode.nMinutes = pSystemExclusive[6];
	m_MidiTimeCode.nSeconds = pSystemExclusive[7];
	m_MidiTimeCode.nFrames = pSystemExclusive[8];
	m_MidiTimeCode.nType = m_nTimeCodeType;

	Update();
}

void MidiReader::HandleMtcQf(void) {
	uint8_t nData1, nData2;

	Midi::Get()->GetMessageData(nData1, nData2);

	const uint8_t nPart = (nData1 & 0x70) >> 4;

	qf[nPart] = nData1 & 0x0F;

	m_nTimeCodeType = (_midi_timecode_type) (qf[7] >> 1);

	if ((nPart == 7) || (m_nPartPrevious == 7)) {
	} else {
		m_bDirection = (m_nPartPrevious < nPart);
	}

	if ((m_bDirection && (nPart == 7)) || (!m_bDirection && (nPart == 0))) {
		itoa_base10(qf[6] | ((qf[7] & 0x1) << 4), (char*) &m_aTimeCode[0]);
		itoa_base10(qf[4] | (qf[5] << 4), (char*) &m_aTimeCode[3]);
		itoa_base10(qf[2] | (qf[3] << 4), (char*) &m_aTimeCode[6]);
		itoa_base10(qf[0] | (qf[1] << 4), (char*) &m_aTimeCode[9]);

		m_MidiTimeCode.nHours = qf[6] | ((qf[7] & 0x1) << 4);
		m_MidiTimeCode.nMinutes = qf[4] | (qf[5] << 4);
		m_MidiTimeCode.nSeconds = qf[2] | (qf[3] << 4);
		m_MidiTimeCode.nFrames = qf[0] | (qf[1] << 4);
		m_MidiTimeCode.nType = m_nTimeCodeType;

		Update();
	}

	m_nPartPrevious = nPart;
}

void MidiReader::Update(void) {
	if (!m_ptLtcDisabledOutputs->bArtNet) {
		ArtNetNode::Get()->SendTimeCode((const struct TArtNetTimeCode *) &m_MidiTimeCode);
	}

	if (!m_ptLtcDisabledOutputs->bRtpMidi) {
		RtpMidi::Get()->SendTimeCode(&m_MidiTimeCode);
	}

	if (!m_ptLtcDisabledOutputs->bNtp) {
		NtpServer::Get()->SetTimeCode((const struct TLtcTimeCode *) &m_MidiTimeCode);
	}

	if (m_nTimeCodeType != m_nTimeCodeTypePrevious) {
		m_nTimeCodeTypePrevious = m_nTimeCodeType;

		if (!m_ptLtcDisabledOutputs->bDisplay) {
			Display::Get()->TextLine(2, (char *) Ltc::GetType((TTimecodeTypes) m_nTimeCodeType), TC_TYPE_MAX_LENGTH);
		}
		LtcLeds::Get()->Show((TTimecodeTypes) m_nTimeCodeType);
	}

	if (!m_ptLtcDisabledOutputs->bDisplay) {
		Display::Get()->TextLine(1, (const char *) m_aTimeCode, TC_CODE_MAX_LENGTH);
	}

	if (!m_ptLtcDisabledOutputs->bMax7219) {
		DisplayMax7219::Get()->Show((const char *) m_aTimeCode);
	} else
		DisplayWS28xx::Get()->Show((const char *) m_aTimeCode);
	
	
}
