/**
 * @file main.c
 *
 */
/* Copyright (C) 2018-2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "hardware.h"
#include "networkbaremetalmacaddress.h"
#include "ledblink.h"

#include "console.h"

#include "identify.h"

#include "dmxgpioparams.h"

#include "rdmresponder.h"
#include "rdmpersonality.h"

#include "rdmdeviceparams.h"

#include "tlc59711dmxparams.h"
#include "tlc59711dmx.h"

#include "ws28xxdmxparams.h"
#include "ws28xxdmx.h"
#include "ws28xx.h"

#include "spiflashinstall.h"
#include "spiflashstore.h"
#include "storerdmdevice.h"
#include "storews28xxdmx.h"
#include "storetlc59711.h"
#include "storerdmdevice.h"

#include "firmwareversion.h"
#include "software_version.h"

extern "C" {

void notmain(void) {
	Hardware hw;
	NetworkBaremetalMacAddress nw;
	LedBlink lb;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);

	SpiFlashInstall spiFlashInstall;
	SpiFlashStore spiFlashStore;

	Identify identify;
	LightSet *pLightSet;

	char aDescription[32];

	fw.Print();

	hw.SetLed(HARDWARE_LED_ON);

	DmxGpioParams dmxgpioparams;

	if (dmxgpioparams.Load()) {
		dmxgpioparams.Dump();
	}

	bool isSet;
	uint8_t nGpioDataDirection = dmxgpioparams.GetDataDirection(isSet); // Returning default GPIO18

	bool isLedTypeSet = false;

	StoreTLC59711 storeTLC59711;
	TLC59711DmxParams pwmledparms((TLC59711DmxParamsStore *) &storeTLC59711);

	if (pwmledparms.Load()) {
		if ((isLedTypeSet = pwmledparms.IsSetLedType()) == true) {
			TLC59711Dmx *pTLC59711Dmx = new TLC59711Dmx;
			assert(pTLC59711Dmx != 0);
			pTLC59711Dmx->SetTLC59711DmxStore((TLC59711DmxStore *) &storeTLC59711);

			pwmledparms.Dump();
			pwmledparms.Set(pTLC59711Dmx);
			snprintf(aDescription, sizeof(aDescription) -1, "%s", TLC59711DmxParams::GetLedTypeString(pTLC59711Dmx->GetLEDType()));
			pLightSet = pTLC59711Dmx;
		}
	}

	StoreWS28xxDmx storeWS28xxDmx;

	if (!isLedTypeSet) {
		WS28xxDmx *pSPISend = new WS28xxDmx;
		assert(pSPISend != 0);
		pSPISend->SetWS28xxDmxStore((WS28xxDmxStore *) &storeWS28xxDmx);

		WS28xxDmxParams deviceparams((WS28xxDmxParamsStore *) &storeWS28xxDmx);

		if (deviceparams.Load()) {
			deviceparams.Dump();
			deviceparams.Set(pSPISend);
		}

		snprintf(aDescription, sizeof(aDescription) -1, "%s", WS28xx::GetLedTypeString(pSPISend->GetLEDType()));
		pLightSet = pSPISend;
	}

	RDMPersonality personality(aDescription, pLightSet->GetDmxFootprint());

	RDMResponder dmxrdm(&personality, pLightSet, nGpioDataDirection);

	StoreRDMDevice storeRdmDevice;
	RDMDeviceParams rdmDeviceParams((RDMDeviceParamsStore *)&storeRdmDevice);
	RDMDevice *pRDMDevice = (RDMDevice *)&dmxrdm;
	pRDMDevice->SetRDMDeviceStore((RDMDeviceStore *)&storeRdmDevice);

	if (rdmDeviceParams.Load()) {
		rdmDeviceParams.Set((RDMDevice *)&dmxrdm);
		rdmDeviceParams.Dump();
	}

	dmxrdm.Init();
	dmxrdm.Print();

	hw.WatchdogInit();

	dmxrdm.SetOutput(pLightSet);
	dmxrdm.Start();

	for(;;) {
		hw.WatchdogFeed();
		dmxrdm.Run();
		spiFlashStore.Flash();
		identify.Run();
		lb.Run();
	}
}

}
