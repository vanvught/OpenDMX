/**
 * @file hardware.h
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

#ifndef LINUX_HARDWARE_H_
#define LINUX_HARDWARE_H_

#include <time.h>
#include <stdint.h>
#include <sys/utsname.h>

#include "hardware.h"

class Hardware {
public:
	Hardware(void);
	~Hardware(void);

	const char* GetMachine(uint8_t &nLength);
	const char* GetSysName(uint8_t &nLength);

	uint32_t GetReleaseId(void);

	const char* GetBoardName(uint8_t &nLength);

	uint32_t GetBoardId(void) {
		return m_nBoardId;
	}

	const char* GetCpuName(uint8_t &nLength);
	const char* GetSocName(uint8_t &nLength);

	float GetCoreTemperature(void);
	float GetCoreTemperatureMax(void);

	void SetLed(THardwareLedStatus tLedStatus);

	bool Reboot(void);
	bool PowerOff(void);

	uint64_t GetUpTime(void);

	time_t GetTime(void) {
		return time(NULL);
	}

	void SetSysTime(time_t nTime);

	bool SetTime(const struct THardwareTime &pTime);
	void GetTime(struct THardwareTime *pTime);

	uint32_t Micros(void);
	uint32_t Millis(void);

	bool IsWatchdog(void) { return false;}
	void WatchdogInit(void) { } // Not implemented
	void WatchdogFeed(void) { } // Not implemented
	void WatchdogStop(void) { } // Not implemented

	const char* GetWebsiteUrl(void) {
		return "www.orangepi-dmx.org";
	}

	TBootDevice GetBootDevice(void) {
#if defined (RASPPI)
		return BOOT_DEVICE_MMC0;
#else
		return BOOT_DEVICE_HDD;
#endif
	}

public:
	 static Hardware* Get(void) {
		return s_pThis;
	}

private:
	bool ExecCmd(const char* pCmd, char *Result, int nResultSize);

private:
	enum TBoardType {
		BOARD_TYPE_LINUX,
		BOARD_TYPE_CYGWIN,
		BOARD_TYPE_RASPBIAN,
		BOARD_TYPE_OSX,
		BOARD_TYPE_UNKNOWN
	};

	TBoardType m_tBoardType;

	struct utsname m_TOsInfo;

	char m_aCpuName[64];
	char m_aSocName[64];
	char m_aBoardName[64];

	uint32_t m_nBoardId;

	static Hardware *s_pThis;
};

#endif /* LINUX_HARDWARE_H_ */
