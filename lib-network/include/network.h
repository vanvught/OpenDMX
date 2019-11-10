/**
 * @file network.h
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

#ifndef NETWORK_H_
#define NETWORK_H_

#include <stdint.h>
#include <stdbool.h>
#include <net/if.h>

#include "networkdisplay.h"
#include "networkstore.h"

enum TNetwork {
	NETWORK_IP_SIZE = 4,
	NETWORK_MAC_SIZE = 6,
	NETWORK_HOSTNAME_SIZE = 64	/* including a terminating null byte. */
};

#ifndef IP2STR
 #define IP2STR(addr) (uint8_t)(addr & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 24) & 0xFF)
 #define IPSTR "%d.%d.%d.%d"
#endif

#ifndef IP2STR3
 #define IP2STR3(addr) (uint8_t)(addr[0]), (uint8_t)(addr[1]), (uint8_t)(addr[2]), (uint8_t)(addr[3])
 #define IPSTR3 "%.3d.%.3d.%.3d.%.3d"
#endif

#ifndef MAC2STR
 #define MAC2STR(mac) (int)(mac[0]),(int)(mac[1]),(int)(mac[2]),(int)(mac[3]), (int)(mac[4]), (int)(mac[5])
 #define MACSTR "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
#endif

class Network {
public:
	Network(void);
	virtual ~Network(void);

	void Print(void);

	virtual int32_t Begin(uint16_t nPort)=0;
	virtual int32_t End(uint16_t nPort)=0;

	virtual void MacAddressCopyTo(uint8_t *pMacAddress)=0;

	virtual void JoinGroup(uint32_t nHandle, uint32_t nIp)=0;
	virtual void LeaveGroup(uint32_t nHandle, uint32_t nIp)=0;

	virtual uint16_t RecvFrom(uint32_t nHandle, uint8_t *pPacket, uint16_t nSize, uint32_t *pFromIp, uint16_t *pFromPort)=0;
	virtual void SendTo(uint32_t nHandle, const uint8_t *pPacket, uint16_t nSize, uint32_t nToIp, uint16_t nRemotePort)=0;

	virtual void SetIp(uint32_t nIp)=0;
	uint32_t GetIp(void) {
		return m_nLocalIp;
	}

	virtual void SetHostName(const char *pHostName);
	const char *GetHostName(void) {
		return m_aHostName;
	}

	bool SetStaticIp(bool bQueueing, uint32_t nLocalIp, uint32_t nNetmask = 0);

	uint32_t GetGatewayIp(void) {
		return m_nGatewayIp;
	}

	virtual void SetNetmask(uint32_t nNetmask)=0;
	uint32_t GetNetmask(void) {
		return m_nNetmask;
	}

	uint32_t GetNetmaskCIDR(void) {
		return __builtin_popcount(m_nNetmask);
	}

	uint32_t GetBroadcastIp(void) {
		return m_nBroadcastIp;
	}

	virtual bool EnableDhcp(void);

	bool IsDhcpCapable(void) {
		return m_IsDhcpCapable;
	}

	bool IsDhcpUsed(void) {
		return m_IsDhcpUsed;
	}

	 bool IsDhcpKnown(void) {
#if defined (__CYGWIN__) || defined (__APPLE__)
		return false;
#else
		return true;
#endif
	}

	const char* GetIfName(void) {
		return m_aIfName;
	}

	uint32_t GetIfIndex(void) {
		return m_nIfIndex;
	}

	uint32_t GetNtpServerIp(void) {
		return m_nNtpServerIp;
	}

	float GetNtpUtcOffset(void) {
		return m_fNtpUtcOffset;
	}

	void SetNetworkDisplay(NetworkDisplay *pNetworkDisplay) {
		m_pNetworkDisplay = pNetworkDisplay;
	}

	void SetNetworkStore(NetworkStore *pNetworkStore) {
		m_pNetworkStore = pNetworkStore;
	}

public:
	static Network* Get(void) {
		return s_pThis;
	}

	static uint32_t CIDRToNetmask(uint8_t nCDIR);

protected:
	uint8_t m_aNetMacaddr[NETWORK_MAC_SIZE];
	uint32_t m_nLocalIp;
	uint32_t m_nGatewayIp;
	uint32_t m_nNetmask;
	uint32_t m_nBroadcastIp;
	bool m_IsDhcpCapable;
	bool m_IsDhcpUsed;
	char m_aHostName[NETWORK_HOSTNAME_SIZE];
	char m_aIfName[IFNAMSIZ];
	uint32_t m_nIfIndex;
	uint32_t m_nNtpServerIp;
	float m_fNtpUtcOffset;

	NetworkDisplay *m_pNetworkDisplay;
	NetworkStore *m_pNetworkStore;

private:
	uint32_t m_nQueuedLocalIp;
	uint32_t m_nQueuedNetmask;

	static Network *s_pThis;
};

#endif /* NETWORK_H_ */
