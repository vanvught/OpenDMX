/**
 * @file main.cpp
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

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "hardware.h"
#include "networkh3emac.h"
#include "ledblink.h"

#include "console.h"
#include "display.h"

#include "networkconst.h"

#include "mdns.h"
#include "mdnsservices.h"

#include "oscserver.h"
#include "oscserverparms.h"

// Addressable led
#include "lightset.h"
#include "ws28xxdmxparams.h"
#include "ws28xxdmx.h"
#include "ws28xxdmxgrouping.h"
#include "ws28xx.h"
#include "handler.h"
#include "storews28xxdmx.h"
// PWM Led
#include "tlc59711dmxparams.h"
#include "tlc59711dmx.h"
#include "handlertlc59711.h"
#include "storetlc59711.h"

#include "spiflashinstall.h"
#include "spiflashstore.h"
#include "storeoscserver.h"
#include "remoteconfig.h"
#include "remoteconfigparams.h"
#include "storeremoteconfig.h"


#include "firmwareversion.h"

#include "software_version.h"

static const char BRIDGE_PARMAS[] = "Setting Bridge parameters ...";
static const char START_BRIDGE[] = "Starting the Bridge ...";
static const char BRIDGE_STARTED[] = "Bridge started";

extern "C" {

void notmain(void) {
	Hardware hw;
	NetworkH3emac nw;
	LedBlink lb;
	Display display(DISPLAY_SSD1306);
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);
	SpiFlashInstall spiFlashInstall;
	SpiFlashStore spiFlashStore;
	StoreOscServer storeOscServer;
	StoreWS28xxDmx storeWS28xxDmx;
	StoreTLC59711 storeTLC59711;

	OSCServerParams params((OSCServerParamsStore *)&storeOscServer);
	OscServer server;

	if (params.Load()) {
		params.Dump();
		params.Set(&server);
	}

	fw.Print();

	hw.SetLed(HARDWARE_LED_ON);

	console_status(CONSOLE_YELLOW, NetworkConst::MSG_NETWORK_INIT);
	display.TextStatus(NetworkConst::MSG_NETWORK_INIT, DISPLAY_7SEGMENT_MSG_INFO_NETWORK_INIT);

	nw.Init((NetworkParamsStore *)spiFlashStore.GetStoreNetwork());
	nw.SetNetworkStore((NetworkStore *)spiFlashStore.GetStoreNetwork());
	nw.Print();

	MDNS mDns;

	mDns.Start();
	mDns.AddServiceRecord(0, MDNS_SERVICE_CONFIG, 0x2905);
	mDns.AddServiceRecord(0, MDNS_SERVICE_OSC, server.GetPortIncoming(), "type=server");
	mDns.Print();

	console_status(CONSOLE_YELLOW, BRIDGE_PARMAS);
	display.TextStatus(BRIDGE_PARMAS, DISPLAY_7SEGMENT_MSG_INFO_BRIDGE_PARMAMS);

	LightSet *pSpi;
	OscServerHandler *pHandler;

	bool isLedTypeSet = false;

	TLC59711DmxParams pwmledparms((TLC59711DmxParamsStore*) &storeTLC59711);

	if (pwmledparms.Load()) {
		if ((isLedTypeSet = pwmledparms.IsSetLedType()) == true) {
			TLC59711Dmx *pTLC59711Dmx = new TLC59711Dmx;
			assert(pTLC59711Dmx != 0);
			pwmledparms.Dump();
			pwmledparms.Set(pTLC59711Dmx);
			pSpi = pTLC59711Dmx;

			pHandler = new HandlerTLC59711(pTLC59711Dmx);
			assert(pHandler != 0);

			display.Printf(7, "%s:%d", pwmledparms.GetLedTypeString(pwmledparms.GetLedType()), pwmledparms.GetLedCount());
		}
	}

	if (!isLedTypeSet) {

		WS28xxDmxParams ws28xxparms((WS28xxDmxParamsStore *) &storeWS28xxDmx);

		if (ws28xxparms.Load()) {
			ws28xxparms.Dump();
		}

		display.Printf(7, "%s:%d %c", WS28xx::GetLedTypeString(ws28xxparms.GetLedType()), ws28xxparms.GetLedCount(), ws28xxparms.IsLedGrouping() ? 'G' : ' ');

		if (ws28xxparms.IsLedGrouping()) {
			WS28xxDmxGrouping *pWS28xxDmxGrouping = new WS28xxDmxGrouping;
			assert(pWS28xxDmxGrouping != 0);
			ws28xxparms.Set(pWS28xxDmxGrouping);
			pSpi = pWS28xxDmxGrouping;

			display.Printf(7, "%s:%d G", WS28xx::GetLedTypeString(ws28xxparms.GetLedType()), ws28xxparms.GetLedCount());

			pHandler = new Handler(pWS28xxDmxGrouping);
			assert(pHandler != 0);
		} else  {
			WS28xxDmx *pWS28xxDmx = new WS28xxDmx;
			assert(pWS28xxDmx != 0);
			ws28xxparms.Set(pWS28xxDmx);
			pSpi = pWS28xxDmx;

			const uint16_t nLedCount = pWS28xxDmx->GetLEDCount();

			// For the time being, just 1 Universe
			if (pWS28xxDmx->GetLEDType() == SK6812W) {
				if (nLedCount > 128) {
					pWS28xxDmx->SetLEDCount(128);
				}
			} else {
				if (nLedCount > 170) {
					pWS28xxDmx->SetLEDCount(170);
				}
			}

			display.Printf(7, "%s:%d", WS28xx::GetLedTypeString(ws28xxparms.GetLedType()), nLedCount);

			pHandler = new Handler(pWS28xxDmx);
			assert(pHandler != 0);
		}
	}

	server.SetOutput(pSpi);
	server.SetOscServerHandler(pHandler);
	server.Print();

	pSpi->Print();

	RemoteConfig remoteConfig(REMOTE_CONFIG_OSC,  REMOTE_CONFIG_MODE_PIXEL, 1);

	StoreRemoteConfig storeRemoteConfig;
	RemoteConfigParams remoteConfigParams(&storeRemoteConfig);

	if(remoteConfigParams.Load()) {
		remoteConfigParams.Set(&remoteConfig);
		remoteConfigParams.Dump();
	}

	for (unsigned i = 1; i < 7 ; i++) {
		display.ClearLine(i);
	}

	uint8_t nHwTextLength;

	display.Printf(1, "Eth OSC Pixel");
	display.Write(2, hw.GetBoardName(nHwTextLength));
	display.Printf(3, "IP: " IPSTR " %c", IP2STR(Network::Get()->GetIp()), nw.IsDhcpKnown() ? (nw.IsDhcpUsed() ? 'D' : 'S') : ' ');
	display.Printf(4, "In: %d", server.GetPortIncoming());
	display.Printf(5, "Out: %d", server.GetPortOutgoing());

	console_status(CONSOLE_YELLOW, START_BRIDGE);
	display.TextStatus(START_BRIDGE, DISPLAY_7SEGMENT_MSG_INFO_BRIDGE_START);

	server.Start();

	hw.SetLed(HARDWARE_LED_FLASH);

	console_status(CONSOLE_GREEN, BRIDGE_STARTED);
	display.TextStatus(BRIDGE_STARTED, DISPLAY_7SEGMENT_MSG_INFO_BRIDGE_STARTED);

	while (spiFlashStore.Flash())
		;

	hw.WatchdogInit();

	for (;;) {
		hw.WatchdogFeed();
		nw.Run();
		server.Run();
		remoteConfig.Run();
		spiFlashStore.Flash();
		mDns.Run();
		lb.Run();
		display.Run();
	}
}
}
