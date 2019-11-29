/**
 * @file storemotors.h
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

#ifndef STOREMOTORS_H_
#define STOREMOTORS_H_

#include <stdint.h>

#include "modeparams.h"
#include "motorparams.h"
#include "l6470params.h"
#include "modestore.h"

class StoreMotors: public ModeParamsStore, MotorParamsStore, L6470ParamsStore, ModeStore {
public:
	StoreMotors(void);
	~StoreMotors(void);

	void Update(uint8_t nMotorIndex, const struct TModeParams *ptModeParams);
	void Copy(uint8_t nMotorIndex, struct TModeParams *ptModeParams);

	void Update(uint8_t nMotorIndex, const struct TMotorParams *ptMotorParams);
	void Copy(uint8_t nMotorIndex, struct TMotorParams *ptMotorParams);

	void Update(uint8_t nMotorIndex, const struct TL6470Params *ptL6470Params);
	void Copy(uint8_t nMotorIndex, struct TL6470Params *ptL6470Params);

	void SaveDmxStartAddress(uint8_t nMotorIndex, uint16_t nDmxStartAddress);

public:
	static StoreMotors *Get(void) {
		return s_pThis;
	}

private:
	static StoreMotors *s_pThis;
};

#endif /* STOREMOTORS_H_ */
