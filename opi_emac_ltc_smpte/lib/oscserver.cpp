/**
 * @file oscserver.cpp
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
#include <stdio.h>
#include <assert.h>

#include "oscserver.h"
#include "osc.h"

#include "network.h"

#include "h3/ltcgenerator.h"

#include "debug.h"

#ifndef ALIGNED
 #define ALIGNED __attribute__ ((aligned (4)))
#endif

enum TOscServerPort {
	OSCSERVER_PORT_DEFAULT_INCOMING = 8000
};

#define OSCSERVER_MAX_BUFFER 		4096

static const char sStart[] ALIGNED = "start";
#define START_LENGTH (sizeof(sStart)/sizeof(sStart[0]) - 1)

static const char sStop[] ALIGNED = "stop";
#define STOP_LENGTH (sizeof(sStop)/sizeof(sStop[0]) - 1)

static const char sResume[] ALIGNED = "resume";
#define RESUME_LENGTH (sizeof(sResume)/sizeof(sResume[0]) - 1)

static const char sSet[] ALIGNED = "/set/";
#define SET_LENGTH (sizeof(sSet)/sizeof(sSet[0]) - 1)

// "hh/mm/ss/ff" -> length = 11
#define VALUE_LENGTH	11

OSCServer::OSCServer(void):
	m_nPortIncoming(OSCSERVER_PORT_DEFAULT_INCOMING),
	m_nHandle(-1),
	m_nRemoteIp(0),
	m_nRemotePort(0),
	m_nPathLength(0)
{
	m_pBuffer = new uint8_t[OSCSERVER_MAX_BUFFER];
	assert(m_pBuffer != 0);

	m_nPathLength = snprintf(m_aPath, sizeof(m_aPath) - 1, "/%s/tc/*", Network::Get()->GetHostName()) - 1;

	DEBUG_PRINTF("%d [%s]", m_nPathLength, m_aPath);
}

OSCServer::~OSCServer(void) {
	delete[] m_pBuffer;
	m_pBuffer = 0;
}

void OSCServer::Start(void) {
	m_nHandle = Network::Get()->Begin(m_nPortIncoming);
	assert(m_nHandle != -1);
}

void OSCServer::Stop(void) {
}

void OSCServer::Run(void) {
	const int nBytesReceived = Network::Get()->RecvFrom(m_nHandle, m_pBuffer, OSCSERVER_MAX_BUFFER, &m_nRemoteIp, &m_nRemotePort);

	if (__builtin_expect((nBytesReceived <= 0), 1)) {
		return;
	}

	if (OSC::isMatch((const char*) m_pBuffer, m_aPath)) {
		const uint32_t nCommandLength = strlen((const char *)m_pBuffer);

		DEBUG_PUTS(m_pBuffer);
		DEBUG_PRINTF("%d,%d %s", (int) nCommandLength, m_nPathLength, &m_pBuffer[m_nPathLength]);

		if (memcmp(&m_pBuffer[m_nPathLength], sStart, START_LENGTH) == 0) {
			if ((nCommandLength == (m_nPathLength + START_LENGTH)) ) {
				LtcGenerator::Get()->ActionStart();

				DEBUG_PUTS("ActionStart");
			} else if ((nCommandLength == (m_nPathLength + START_LENGTH + 1 + VALUE_LENGTH))) {
				if (m_pBuffer[m_nPathLength + START_LENGTH] == '/') {
					const uint32_t nOffset = m_nPathLength + START_LENGTH + 1;
					m_pBuffer[nOffset + 2] = ':';
					m_pBuffer[nOffset + 5] = ':';
					m_pBuffer[nOffset + 8] = '.';

					LtcGenerator::Get()->ActionSetStart((const char *)&m_pBuffer[nOffset]);
					LtcGenerator::Get()->ActionStop();
					LtcGenerator::Get()->ActionStart();

					DEBUG_PUTS(&m_pBuffer[nOffset]);
				}
			} else if ((nCommandLength == (m_nPathLength + START_LENGTH + SET_LENGTH + VALUE_LENGTH))) {
				if (memcmp(&m_pBuffer[m_nPathLength + START_LENGTH], sSet, SET_LENGTH) == 0) {
					const uint32_t nOffset = m_nPathLength + START_LENGTH + SET_LENGTH;
					m_pBuffer[nOffset + 2] = ':';
					m_pBuffer[nOffset + 5] = ':';
					m_pBuffer[nOffset + 8] = '.';

					LtcGenerator::Get()->ActionSetStart((const char *)&m_pBuffer[nOffset]);

					DEBUG_PUTS(&m_pBuffer[nOffset]);
				}
			}
		} else if (memcmp(&m_pBuffer[m_nPathLength], sStop, STOP_LENGTH) == 0) {
			if ((nCommandLength == (m_nPathLength + STOP_LENGTH))) {
				LtcGenerator::Get()->ActionStop();
				DEBUG_PUTS("ActionStop");
			} else if ((nCommandLength == (m_nPathLength + STOP_LENGTH + SET_LENGTH + VALUE_LENGTH))) {
				if (memcmp(&m_pBuffer[m_nPathLength + STOP_LENGTH], sSet, SET_LENGTH) == 0) {
					const uint32_t nOffset = m_nPathLength + STOP_LENGTH + SET_LENGTH;
					m_pBuffer[nOffset + 2] = ':';
					m_pBuffer[nOffset + 5] = ':';
					m_pBuffer[nOffset + 8] = '.';

					LtcGenerator::Get()->ActionSetStop((const char *)&m_pBuffer[nOffset]);

					DEBUG_PUTS(&m_pBuffer[nOffset]);
				}
			}
		} else if ( (nCommandLength == (m_nPathLength + RESUME_LENGTH)) && (memcmp(&m_pBuffer[m_nPathLength], sResume, RESUME_LENGTH) == 0)) {
			LtcGenerator::Get()->ActionResume();

			DEBUG_PUTS("ActionResume");
		}
	}
}

void OSCServer::Print(void) {
	printf("OSC\n");
	printf(" Port : %d\n", m_nPortIncoming);
	printf(" Path : [%s]\n", m_aPath);
}
