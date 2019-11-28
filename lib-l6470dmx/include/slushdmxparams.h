/**
 * @file slushdmxparams.h
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

#ifndef SLUSHDMXPARAMS_H_
#define SLUSHDMXPARAMS_H_

#include <stdint.h>

#include "slushdmx.h"

struct TSlushDmxParams {
	uint32_t nSetList;
	//
	uint8_t nUseSpiBusy;
	uint16_t nDmxStartAddressPortA;
	uint8_t nDmxFootprintPortA;
	uint16_t nDmxStartAddressPortB;
	uint8_t nDmxFootprintPortB;
}__attribute__((packed));

enum TSlushDmxParamsMask {
	SLUSH_DMX_PARAMS_MASK_USE_SPI_BUSY = (1 << 0),
	SLUSH_DMX_PARAMS_MASK_START_ADDRESS_PORT_A = (1 << 1),
	SLUSH_DMX_PARAMS_MASK_FOOTPRINT_PORT_A = (1 << 2),
	SLUSH_DMX_PARAMS_MASK_START_ADDRESS_PORT_B = (1 << 3),
	SLUSH_DMX_PARAMS_MASK_FOOTPRINT_PORT_B = (1 << 4)
};

class SlushDmxParamsStore {
public:
	virtual ~SlushDmxParamsStore(void);

	virtual void Update(const struct TSlushDmxParams *ptSlushDmxParams)=0;
	virtual void Copy(struct TSlushDmxParams *ptSlushDmxParams)=0;

	virtual void Update(uint8_t nMotorIndex, const struct TSlushDmxParams *ptSlushDmxParams)=0;
	virtual void Copy(uint8_t nMotorIndex, struct TSlushDmxParams *ptSlushDmxParams)=0;
};

class SlushDmxParams {
public:
	SlushDmxParams(SlushDmxParamsStore *pSlushDmxParamsStore=0);
	~SlushDmxParams(void);

	bool Load(void);
	void Load(const char *pBuffer, uint32_t nLength);

	void Set(SlushDmx *pSlushDmx);

	void Builder(const struct TSlushDmxParams *ptSlushDmxParams, uint8_t *pBuffer, uint32_t nLength, uint32_t &nSize);
	void Save(uint8_t *pBuffer, uint32_t nLength, uint32_t &nSize);

	void Dump(void);

public:
    static void staticCallbackFunction(void *p, const char *s);

private:
    void callbackFunction(const char *pLine);
	bool isMaskSet(uint16_t nMask) const;

private:
	SlushDmxParamsStore *m_pSlushDmxParamsStore;
    struct TSlushDmxParams m_tSlushDmxParams;
    char m_aFileName[16];
};

#endif /* SLUSHDMXPARAMS_H_ */
