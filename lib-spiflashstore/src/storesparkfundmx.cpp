/**
 * @file storesparkfundmx.cpp
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
#include <assert.h>

#include "storesparkfundmx.h"

#include "sparkfundmxparams.h"

#include "spiflashstore.h"

#include "debug.h"

StoreSparkFunDmx *StoreSparkFunDmx::s_pThis = 0;

#define STORE_SPARKFUN_MAX_SIZE			96
#define STORE_SPARKFUN_MAX_MOTORS		8
#define STORE_SPARKFUN_STRUCT_OFFSET	16
#define STORE_SPARKFUN_STRUCT_SIZE		(STORE_SPARKFUN_MAX_MOTORS * sizeof(struct TSparkFunDmxParams))

#define STORE_SPARKFUN_OFFSET(x)		(STORE_SPARKFUN_STRUCT_OFFSET + ((x) * sizeof(struct TSparkFunDmxParams)))

SparkFunDmxParamsStore::~SparkFunDmxParamsStore(void) {
	DEBUG_ENTRY

	DEBUG_EXIT
}

StoreSparkFunDmx::StoreSparkFunDmx(void) {
	DEBUG_ENTRY

	s_pThis = this;

	DEBUG_PRINTF("%p", s_pThis);
	DEBUG_PRINTF("sizeof(TSparkFunDmxParams)=%d", (int) sizeof(struct TSparkFunDmxParams));
	DEBUG_PRINTF("STORE_SPARKFUN_STRUCT_OFFSET+STORE_SPARKFUN_STRUCT_SIZE=%d", STORE_SPARKFUN_STRUCT_OFFSET + STORE_SPARKFUN_STRUCT_SIZE);

	assert(sizeof(struct TSparkFunDmxParams) <= STORE_SPARKFUN_STRUCT_OFFSET);
	assert((STORE_SPARKFUN_STRUCT_OFFSET + STORE_SPARKFUN_STRUCT_SIZE) <= STORE_SPARKFUN_MAX_SIZE);

	for (uint32_t nMotorIndex = 0; nMotorIndex < STORE_SPARKFUN_MAX_MOTORS; nMotorIndex++) {
		struct TSparkFunDmxParams tSparkFunDmxParams;
		memset((void *) &tSparkFunDmxParams, 0xFF, sizeof(struct TSparkFunDmxParams));
		tSparkFunDmxParams.nSetList = 0;

		Copy(nMotorIndex, &tSparkFunDmxParams);

		if (tSparkFunDmxParams.nSetList == (uint32_t)~0) {
			DEBUG_PRINTF("%d: Clear nSetList", nMotorIndex);
			tSparkFunDmxParams.nSetList = 0;
			Update(nMotorIndex, &tSparkFunDmxParams);
		}
	}

	DEBUG_EXIT
}

StoreSparkFunDmx::~StoreSparkFunDmx(void) {
	DEBUG_ENTRY

	DEBUG_EXIT
}

void StoreSparkFunDmx::Update(const struct TSparkFunDmxParams *pSparkFunDmxParams) {
	DEBUG_ENTRY

	SpiFlashStore::Get()->Update(STORE_SPARKFUN, (void *)pSparkFunDmxParams, sizeof(struct TSparkFunDmxParams));

	DEBUG_EXIT
}

void StoreSparkFunDmx::Copy(struct TSparkFunDmxParams *pSparkFunDmxParams) {
	DEBUG_ENTRY

	SpiFlashStore::Get()->Copy(STORE_SPARKFUN, (void *)pSparkFunDmxParams, sizeof(struct TSparkFunDmxParams));

	DEBUG_EXIT
}

void StoreSparkFunDmx::Update(uint8_t nMotorIndex, const struct TSparkFunDmxParams *ptSparkFunDmxParams) {
	DEBUG_ENTRY

	assert(nMotorIndex < STORE_SPARKFUN_MAX_MOTORS);

	SpiFlashStore::Get()->Update(STORE_SPARKFUN, STORE_SPARKFUN_OFFSET(nMotorIndex), (void *)ptSparkFunDmxParams, sizeof(struct TSparkFunDmxParams));

	DEBUG_EXIT
}

void StoreSparkFunDmx::Copy(uint8_t nMotorIndex, struct TSparkFunDmxParams *ptSparkFunDmxParams) {
	DEBUG_ENTRY

	assert(nMotorIndex < STORE_SPARKFUN_MAX_MOTORS);

	SpiFlashStore::Get()->Copy(STORE_SPARKFUN, (void *)ptSparkFunDmxParams, sizeof(struct TSparkFunDmxParams), STORE_SPARKFUN_OFFSET(nMotorIndex));

	DEBUG_EXIT
}
