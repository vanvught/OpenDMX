/**
 * @file sparkfundmx.h
 *
 */
/* Copyright (C) 2017-2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#ifndef SPARKFUNDMX_H_
#define SPARKFUNDMX_H_

#include <stdint.h>
#include <stdbool.h>

#include "lightset.h"
#include "l6470dmxmodes.h"

#include "modeparams.h"
#include "motorparams.h"
#include "l6470params.h"

#include "autodriver.h"

#include "hal_spi.h"
#include "hal_gpio.h"

#include "modestore.h"

#define SPARKFUN_DMX_MAX_MOTORS		8

#if !defined (H3)
 #define GPIO_BUSY_IN		GPIO_EXT_35
 #define GPIO_RESET_OUT 	GPIO_EXT_38
#else
 #define GPIO_BUSY_IN		GPIO_EXT_11
 #define GPIO_RESET_OUT 	GPIO_EXT_13
#endif

struct TSparkFunStores {
	void *pSparkFunDmxParamsStore;
	ModeParamsStore *pModeParamsStore;
	MotorParamsStore *pMotorParamsStore;
	L6470ParamsStore *pL6470ParamsStore;
};

class SparkFunDmx: public LightSet {
public:
	SparkFunDmx(void);
	~SparkFunDmx(void);

	void Start(uint8_t nPort);
	void Stop(uint8_t nPort);

	void SetData(uint8_t nPort, const uint8_t *, uint16_t);

	void Print(void);

	uint32_t GetMotorsConnected(void) {
		return AutoDriver::getNumBoards();
	}

	void SetModeStore(ModeStore *pModeStore) {
		m_pModeStore = pModeStore;
	}

// RDM
	bool SetDmxStartAddress(uint16_t nDmxStartAddress);
	uint16_t GetDmxStartAddress(void) {
		return m_nDmxStartAddress;
	}

	uint16_t GetDmxFootprint(void) {
		return m_nDmxFootprint;
	}

	bool GetSlotInfo(uint16_t nSlotOffset, struct TLightSetSlotInfo &tSlotInfo);

//
	void SetGlobalSpiCs(uint8_t nSpiCs) {
		m_nGlobalSpiCs = nSpiCs;
		m_bIsGlobalSpiCsSet = true;
	}

	void SetGlobalResetPin(uint8_t nResetPin) {
		m_nGlobalResetPin = nResetPin;
		m_bIsGlobalResetSet = true;
	}

	void SetGlobalBusyPin(uint8_t nBusyPin) {
		m_nGlobalBusyPin = nBusyPin;
		m_bIsGlobalBusyPinSet = true;
	}

	void SetLocalPosition(uint8_t nPosition) {
		m_nLocalPosition = nPosition;
		m_bIsLocalPositionSet = true;
	}

	void SetLocalSpiCs(uint8_t nSpiCs) {
		m_nLocalSpiCs = nSpiCs;
		m_bIsLocalSpiCsSet = true;
	}

	void SetLocalResetPin(uint8_t nResetPin) {
		m_nLocalResetPin = nResetPin;
		m_bIsLocalResetSet = true;
	}

	void SetLocalBusyPin(uint8_t nBusyPin) {
		m_nLocalBusyPin = nBusyPin;
		m_bIsLocalBusyPinSet = true;
	}

public:
	void ReadConfigFiles(struct TSparkFunStores *ptSparkFunStores=0);

private:
	AutoDriver *m_pAutoDriver[SPARKFUN_DMX_MAX_MOTORS];
	MotorParams *m_pMotorParams[SPARKFUN_DMX_MAX_MOTORS];
	ModeParams *m_pModeParams[SPARKFUN_DMX_MAX_MOTORS];
	L6470DmxModes *m_pL6470DmxModes[SPARKFUN_DMX_MAX_MOTORS];
	struct TLightSetSlotInfo *m_pSlotInfo[SPARKFUN_DMX_MAX_MOTORS];

	uint8_t m_nGlobalSpiCs;
	uint8_t m_nGlobalResetPin;
	uint8_t m_nGlobalBusyPin;

	bool m_bIsGlobalSpiCsSet;
	bool m_bIsGlobalResetSet;
	bool m_bIsGlobalBusyPinSet;

	uint8_t m_nLocalPosition;
	uint8_t m_nLocalSpiCs;
	uint8_t m_nLocalResetPin;
	uint8_t m_nLocalBusyPin;

	bool m_bIsLocalPositionSet;
	bool m_bIsLocalSpiCsSet;
	bool m_bIsLocalResetSet;
	bool m_bIsLocalBusyPinSet;

	uint8_t m_nDmxMode;
	uint16_t m_nDmxStartAddressMode;

	uint16_t m_nDmxStartAddress;
	uint16_t m_nDmxFootprint;

	ModeStore *m_pModeStore;
};

#endif /* SPARKFUNDMX_H_ */
