/**
 * @file firmwareversion.cpp
 *
 */
/* Copyright (C) 2020-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "firmwareversion.h"

#include "hardware.h"

FirmwareVersion::FirmwareVersion(const char *pSoftwareVersion, const char *pDate, const char *pTime, const uint32_t nSoftwareVersionId) : s_nSoftwareVersionId(nSoftwareVersionId)
{
	assert(pSoftwareVersion != nullptr);
	assert(pDate != nullptr);
	assert(pTime != nullptr);

	assert(s_pThis == nullptr);
	s_pThis = this;

	memcpy(s_FirmwareVersion.SoftwareVersion, pSoftwareVersion, firmwareversion::length::SOFTWARE_VERSION);
	memcpy(s_FirmwareVersion.BuildDate, pDate, firmwareversion::length::GCC_DATE);
	memcpy(s_FirmwareVersion.BuildTime, pTime, firmwareversion::length::GCC_TIME);

	uint8_t nHwTextLength;

	snprintf(s_Print, sizeof(s_Print) - 1, "[V%.*s] %s Compiled on %.*s at %.*s",
			firmwareversion::length::SOFTWARE_VERSION, s_FirmwareVersion.SoftwareVersion,
			Hardware::Get()->GetBoardName(nHwTextLength),
			firmwareversion::length::GCC_DATE, s_FirmwareVersion.BuildDate,
			firmwareversion::length::GCC_TIME, s_FirmwareVersion.BuildTime);
}
