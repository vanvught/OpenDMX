/**
 * @file main.cpp
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
#include "networkh3emac.h"
#include "ledblink.h"

#include "console.h"

#include "displayudf.h"
#include "displayudfparams.h"
#include "storedisplayudf.h"

#include "networkconst.h"
#include "artnetconst.h"

#include "artnet4node.h"
#include "artnet4params.h"

#include "ipprog.h"
#include "displayudfhandler.h"

// Addressable led
#include "lightset.h"
#include "ws28xxdmxparams.h"
#include "ws28xxdmx.h"
#include "ws28xxdmxgrouping.h"
#include "ws28xx.h"
#include "storews28xxdmx.h"
// PWM Led
#include "tlc59711dmxparams.h"
#include "tlc59711dmx.h"
#include "storetlc59711.h"

#include "spiflashinstall.h"
#include "spiflashstore.h"
#include "remoteconfig.h"
#include "remoteconfigparams.h"
#include "storeremoteconfig.h"

#include "firmwareversion.h"

#include "software_version.h"

extern "C" {

void notmain(void) {
	Hardware hw;
	NetworkH3emac nw;
	LedBlink lb;
	DisplayUdf display;
	DisplayUdfHandler displayUdfHandler;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);

	SpiFlashInstall spiFlashInstall;
	SpiFlashStore spiFlashStore;

	StoreWS28xxDmx storeWS28xxDmx;
	StoreTLC59711 storeTLC59711;

	ArtNet4Params artnetparams((ArtNet4ParamsStore *)spiFlashStore.GetStoreArtNet4());

	if (artnetparams.Load()) {
		artnetparams.Dump();
	}

	fw.Print();

	console_puts("Ethernet Art-Net 4 Node Pixel controller {4 Universes}\n");

	hw.SetLed(HARDWARE_LED_ON);

	console_status(CONSOLE_YELLOW, NetworkConst::MSG_NETWORK_INIT);
	display.TextStatus(NetworkConst::MSG_NETWORK_INIT, DISPLAY_7SEGMENT_MSG_INFO_NETWORK_INIT);

	nw.Init((NetworkParamsStore *)spiFlashStore.GetStoreNetwork());
	nw.SetNetworkStore((NetworkStore *)spiFlashStore.GetStoreNetwork());
	nw.SetNetworkDisplay((NetworkDisplay *)&displayUdfHandler);
	nw.Print();

	console_status(CONSOLE_YELLOW, ArtNetConst::MSG_NODE_PARAMS);
	display.TextStatus(ArtNetConst::MSG_NODE_PARAMS, DISPLAY_7SEGMENT_MSG_INFO_NODE_PARMAMS);

	ArtNet4Node node;
	artnetparams.Set(&node);

	IpProg ipprog;
	node.SetIpProgHandler(&ipprog);

	node.SetArtNetDisplay((ArtNetDisplay *)&displayUdfHandler);
	node.SetArtNetStore((ArtNetStore *)spiFlashStore.GetStoreArtNet());

	const uint8_t nUniverse = artnetparams.GetUniverse();

	node.SetUniverseSwitch(0, ARTNET_OUTPUT_PORT, nUniverse);

	LightSet *pSpi;

	bool isLedTypeSet = false;

	TLC59711DmxParams pwmledparms((TLC59711DmxParamsStore *) &storeTLC59711);

	if (pwmledparms.Load()) {
		if ((isLedTypeSet = pwmledparms.IsSetLedType()) == true) {
			TLC59711Dmx *pTLC59711Dmx = new TLC59711Dmx;
			assert(pTLC59711Dmx != 0);
			pwmledparms.Dump();
			pwmledparms.Set(pTLC59711Dmx);
			pSpi = pTLC59711Dmx;
			display.Printf(7, "%s:%d", pwmledparms.GetLedTypeString(pwmledparms.GetLedType()), pwmledparms.GetLedCount());
		}
	}

	if (!isLedTypeSet) {

		WS28xxDmxParams ws28xxparms((WS28xxDmxParamsStore *) &storeWS28xxDmx);

		if (ws28xxparms.Load()) {
			ws28xxparms.Dump();
		}

		const bool bIsLedGrouping = ws28xxparms.IsLedGrouping() && (ws28xxparms.GetLedGroupCount() > 1);

		if (bIsLedGrouping) {
			WS28xxDmxGrouping *pWS28xxDmxGrouping = new WS28xxDmxGrouping;
			assert(pWS28xxDmxGrouping != 0);
			ws28xxparms.Set(pWS28xxDmxGrouping);
			pWS28xxDmxGrouping->SetLEDGroupCount(ws28xxparms.GetLedGroupCount());
			pSpi = pWS28xxDmxGrouping;
			display.Printf(7, "%s:%d G%d", WS28xx::GetLedTypeString(pWS28xxDmxGrouping->GetLEDType()), pWS28xxDmxGrouping->GetLEDCount(), pWS28xxDmxGrouping->GetLEDGroupCount());
		} else  {
			WS28xxDmx *pWS28xxDmx = new WS28xxDmx;
			assert(pWS28xxDmx != 0);
			ws28xxparms.Set(pWS28xxDmx);
			pSpi = pWS28xxDmx;
			display.Printf(7, "%s:%d", WS28xx::GetLedTypeString(pWS28xxDmx->GetLEDType()), pWS28xxDmx->GetLEDCount());

			const uint16_t nLedCount = pWS28xxDmx->GetLEDCount();

			if (pWS28xxDmx->GetLEDType() == SK6812W) {
				if (nLedCount > 128) {
					node.SetDirectUpdate(true);
					node.SetUniverseSwitch(1, ARTNET_OUTPUT_PORT, nUniverse + 1);
				}
				if (nLedCount > 256) {
					node.SetUniverseSwitch(2, ARTNET_OUTPUT_PORT, nUniverse + 2);
				}
				if (nLedCount > 384) {
					node.SetUniverseSwitch(3, ARTNET_OUTPUT_PORT, nUniverse + 3);
				}
			} else {
				if (nLedCount > 170) {
					node.SetDirectUpdate(true);
					node.SetUniverseSwitch(1, ARTNET_OUTPUT_PORT, nUniverse + 1);
				}
				if (nLedCount > 340) {
					node.SetUniverseSwitch(2, ARTNET_OUTPUT_PORT, nUniverse + 2);
				}
				if (nLedCount > 510) {
					node.SetUniverseSwitch(3, ARTNET_OUTPUT_PORT, nUniverse + 3);
				}
			}
		}
	}

	node.SetOutput(pSpi);
	node.Print();

	pSpi->Print();

	display.SetTitle("Eth Art-Net 4 Pixel");
	display.Set(2, DISPLAY_UDF_LABEL_NODE_NAME);
	display.Set(3, DISPLAY_UDF_LABEL_IP);
	display.Set(4, DISPLAY_UDF_LABEL_VERSION);
	display.Set(5, DISPLAY_UDF_LABEL_UNIVERSE);
	display.Set(6, DISPLAY_UDF_LABEL_AP);

	StoreDisplayUdf storeDisplayUdf;
	DisplayUdfParams displayUdfParams(&storeDisplayUdf);

	if(displayUdfParams.Load()) {
		displayUdfParams.Set(&display);
		displayUdfParams.Dump();
	}

	display.Show(&node);

	RemoteConfig remoteConfig(REMOTE_CONFIG_ARTNET, REMOTE_CONFIG_MODE_PIXEL, node.GetActiveOutputPorts());

	StoreRemoteConfig storeRemoteConfig;
	RemoteConfigParams remoteConfigParams(&storeRemoteConfig);

	if(remoteConfigParams.Load()) {
		remoteConfigParams.Set(&remoteConfig);
		remoteConfigParams.Dump();
	}

	while (spiFlashStore.Flash())
		;

	console_status(CONSOLE_YELLOW, ArtNetConst::MSG_NODE_START);
	display.TextStatus(ArtNetConst::MSG_NODE_START, DISPLAY_7SEGMENT_MSG_INFO_NODE_START);

	node.Start();

	console_status(CONSOLE_GREEN, ArtNetConst::MSG_NODE_STARTED);
	display.TextStatus(ArtNetConst::MSG_NODE_STARTED, DISPLAY_7SEGMENT_MSG_INFO_NODE_STARTED);

	hw.WatchdogInit();

	for (;;) {
		hw.WatchdogFeed();
		nw.Run();
		node.Run();
		remoteConfig.Run();
		spiFlashStore.Flash();
		lb.Run();
		display.Run();
	}
}

}
