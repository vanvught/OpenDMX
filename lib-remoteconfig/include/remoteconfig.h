/**
 * @file remoteconfig.h
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

#ifndef REMOTECONFIG_H_
#define REMOTECONFIG_H_

#include <stdint.h>

#if defined (ARTNET_NODE_MULTI)
 #define ARTNET_NODE
#endif

#if defined (E131_BRIDGE_MULTI)
 #define E131_BRIDGE
#endif

#if defined (PIXEL_MULTI)
 #define PIXEL
#endif

#if defined (DMXSEND_MULTI)
 #define DMXSEND
#endif

#include "spiflashstore.h"

#include "tftpfileserver.h"

enum TRemoteConfig {
	REMOTE_CONFIG_ARTNET,
	REMOTE_CONFIG_E131,
	REMOTE_CONFIG_OSC,
	REMOTE_CONFIG_LTC,
	REMOTE_CONFIG_OSC_CLIENT,
	REMOTE_CONFIG_RDMNET_LLRP_ONLY,
	REMOTE_CONFIG_LAST
};

enum TRemoteConfigMode {
	REMOTE_CONFIG_MODE_DMX,
	REMOTE_CONFIG_MODE_RDM,
	REMOTE_CONFIG_MODE_MONITOR,
	REMOTE_CONFIG_MODE_PIXEL,
	REMOTE_CONFIG_MODE_TIMECODE,
	REMOTE_CONFIG_MODE_OSC,
	REMOTE_CONFIG_MODE_CONFIG,
	REMOTE_CONFIG_MODE_STEPPER,
	REMOTE_CONFIG_MODE_LAST
};

enum TTxtFile {
	TXT_FILE_RCONFIG,
	TXT_FILE_NETWORK,
	TXT_FILE_ARTNET,
	TXT_FILE_E131,
	TXT_FILE_OSC,
	TXT_FILE_PARAMS,
	TXT_FILE_DEVICES,
	TXT_FILE_LTC,
	TXT_FILE_TCNET,
	TXT_FILE_OSC_CLIENT,
	TXT_FILE_DISPLAY_UDF,
	TXT_FILE_LTCDISPLAY,
	TXT_FILE_DISPLAY_NEXTION,
#if defined(STEPPER)
	TXT_FILE_SPARKFUN,
	TXT_FILE_MOTOR0,
	TXT_FILE_MOTOR1,
	TXT_FILE_MOTOR2,
	TXT_FILE_MOTOR3,
	TXT_FILE_MOTOR4,
	TXT_FILE_MOTOR5,
	TXT_FILE_MOTOR6,
	TXT_FILE_MOTOR7,
#endif
	TXT_FILE_RDM,
	TXT_FILE_LAST
};

enum {
	REMOTE_CONFIG_DISPLAY_NAME_LENGTH = 24,
	REMOTE_CONFIG_ID_LENGTH = (32 + REMOTE_CONFIG_DISPLAY_NAME_LENGTH + 2) // +2, comma and \n
};

enum TRemoteConfigHandleMode {
	REMOTE_CONFIG_HANDLE_MODE_TXT,
	REMOTE_CONFIG_HANDLE_MODE_BIN
};

struct TRemoteConfigListBin {
	uint8_t aMacAddress[6];
	uint8_t nType;				// TRemoteConfig
	uint8_t nMode;				// TRemoteConfigMode
	uint8_t nActiveUniverses;
	uint8_t aDisplayName[REMOTE_CONFIG_DISPLAY_NAME_LENGTH];
}__attribute__((packed));

class RemoteConfig {
public:
	RemoteConfig(TRemoteConfig tRemoteConfig, TRemoteConfigMode tRemoteConfigMode, uint8_t nOutputs = 0);
	~RemoteConfig(void);

	void SetDisable(bool bDisable = true);
	bool GetDisable(void) {
		return m_bDisable;
	}

	void SetDisableWrite(bool bDisableWrite) {
		m_bDisableWrite = bDisableWrite;
	}
	bool GetDisableWrite(void) {
		return m_bDisableWrite;
	}

	void SetEnableReboot(bool bEnableReboot) {
		m_bEnableReboot = bEnableReboot;
	}
	bool GetEnableReboot(void) {
		return m_bEnableReboot;
	}

	void SetEnableUptime(bool bEnableUptime) {
		m_bEnableUptime = bEnableUptime;
	}
	bool GetEnableUptime(void) {
		return m_bEnableUptime;
	}

	void SetDisplayName(const char *pDisplayName);

	int Run(void);

public:
	static uint32_t GetIndex(const void *p, uint32_t &nLength);
	static TStore GetStore(TTxtFile tTxtFile);
	static void HandleReboot(void);

private:
	void HandleList(void);
	void HandleUptime(void);
	void HandleVersion(void);

	void HandleGet(void);
	void HandleGetRconfigTxt(uint32_t& nSize);
	void HandleGetNetworkTxt(uint32_t& nSize);

#if defined (ARTNET_NODE)
	void HandleGetArtnetTxt(uint32_t& nSize);
#endif
#if defined (E131_BRIDGE)
	void HandleGetE131Txt(uint32_t& nSize);
#endif
#if defined (OSC_SERVER)
	void HandleGetOscTxt(uint32_t& nSize);
#endif
#if defined (DMXSEND)
	void HandleGetParamsTxt(uint32_t& nSize);
#endif
#if defined (PIXEL)
	void HandleGetDevicesTxt(uint32_t& nSize);
#endif
#if defined (LTC_READER)
	void HandleGetLtcTxt(uint32_t& nSize);
	void HandleGetLtcDisplayTxt(uint32_t& nSize);
	void HandleGetTCNetTxt(uint32_t& nSize);
#endif
#if defined (OSC_CLIENT)
	void HandleGetOscClntTxt(uint32_t& nSize);
#endif
#if defined(DISPLAY_UDF)
	void HandleGetDisplayTxt(uint32_t& nSize);
#endif
#if defined(DISPLAY_NEXTION)
	void HandleGetNextionTxt(uint32_t& nSize);
#endif
#if defined(STEPPER)
	void HandleGetSparkFunTxt(uint32_t& nSize);
	void HandleGetMotorTxt(uint32_t nMotorIndex, uint32_t& nSize);
#endif
#if defined(RDM_RESPONDER)
	void HandleGetRdmTxt(uint32_t& nSize);
#endif

	void HandleTxtFile(void);
	void HandleTxtFileRconfig(void);
	void HandleTxtFileNetwork(void);

#if defined (ARTNET_NODE)
	void HandleTxtFileArtnet(void);
#endif
#if defined (E131_BRIDGE)
	void HandleTxtFileE131(void);
#endif
#if defined (OSC_SERVER)
	void HandleTxtFileOsc(void);
#endif
#if defined (DMXSEND)
	void HandleTxtFileParams(void);
#endif
#if defined (PIXEL)
	void HandleTxtFileDevices(void);
#endif
#if defined (LTC_READER)
	void HandleTxtFileLtc(void);
	void HandleTxtFileLtcDisplay(void);
	void HandleTxtFileTCNet(void);
#endif
#if defined (OSC_CLIENT)
	void HandleTxtFileOscClient(void);
#endif
#if defined(DISPLAY_UDF)
	void HandleTxtFileDisplay(void);
#endif
#if defined(DISPLAY_NEXTION)
	void HandleTxtFileNextion(void);
#endif
#if defined(STEPPER)
	void HandleTxtFileSparkFun(void);
	void HandleTxtFileMotor(uint32_t nMotorIndex);
#endif
#if defined(RDM_RESPONDER)
	void HandleTxtFileRdm(void);
#endif

	void HandleDisplaySet(void);
	void HandleDisplayGet(void);

	void HandleStoreSet(void);
	void HandleStoreGet(void);

	void HandleTftpSet(void);
	void HandleTftpGet(void);

public:
	static RemoteConfig* Get(void) {
		return s_pThis;
	}

private:
	static RemoteConfig *s_pThis;

private:
	TRemoteConfig m_tRemoteConfig;
	TRemoteConfigMode m_tRemoteConfigMode;
	uint8_t m_nOutputs;
	bool m_bDisable;
	bool m_bDisableWrite;
	bool m_bEnableReboot;
	bool m_bEnableUptime;
	bool m_bEnableTFTP;
	TFTPFileServer* m_pTFTPFileServer;
	uint8_t *m_pTFTPBuffer;
	char m_aId[REMOTE_CONFIG_ID_LENGTH];
	uint32_t m_nIdLength;
	struct TRemoteConfigListBin m_tRemoteConfigListBin;
	int32_t m_nHandle;
	uint8_t *m_pUdpBuffer;
	uint32_t m_nIPAddressFrom;
	uint16_t m_nBytesReceived;
	TRemoteConfigHandleMode m_tRemoteConfigHandleMode;
	uint8_t *m_pStoreBuffer;
};

#endif /* REMOTECONFIG_H_ */
