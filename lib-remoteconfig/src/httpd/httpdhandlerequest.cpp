/**
 * @file httpdhandlerequest.cpp
 *
 */
/* Copyright (C) 2024-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined (DEBUG_HTTPD)
# undef NDEBUG
#endif

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <cassert>

#include "httpd/httpdhandlerequest.h"
#include "http_status_messages.h"
#include "../http/content/json_switch.h"

#include "remoteconfig.h"
#include "remoteconfigjson.h"
#include "properties.h"
#include "sscan.h"
#include "propertiesconfig.h"

#include "hardware.h"
#include "network.h"
#include "net/tcp.h"
#include "net/apps/mdns.h"
#include "display.h"

#if defined (NODE_ARTNET) && defined(RDM_CONTROLLER)
# include "artnetnode.h"
#endif

#if !defined (CONFIG_HTTP_HTML_NO_DMX)
# if defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI)
#  define HAVE_DMX
# endif
# if defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI)
#  define HAVE_PIXEL
# endif
#endif

#if defined (NODE_SHOWFILE)
# define ENABLE_METHOD_DELETE
#endif

#if defined (ENABLE_FIRMWARE_UPLOAD)
# include "firmware.h"
#endif

#include "debug.h"

#if defined ENABLE_CONTENT
const char *get_file_content(const char *fileName, uint32_t& nSize, http::contentTypes& contentType);
#endif

#ifndef NDEBUG
static constexpr char s_request_method[][8] = {"GET", "POST", "DELETE", "UNKNOWN" };
#endif

static constexpr char s_contentType[static_cast<uint32_t>(http::contentTypes::NOT_DEFINED)][32] =
{ "text/html", "text/css", "text/javascript", "application/json", "application/octet-stream" };

void HttpDeamonHandleRequest::HandleRequest(const uint32_t nBytesReceived, char *pReceiveBuffer) {
	DEBUG_ENTRY

	m_nBytesReceived = nBytesReceived;
	m_pReceiveBuffer = pReceiveBuffer;

	const char *pStatusMsg = "OK";

	DEBUG_PRINTF("%u: m_Status=%u", m_nConnectionHandle, static_cast<uint32_t>(m_Status));

	if (m_Status == http::Status::UNKNOWN_ERROR) {
		// This is an initial incoming HTTP request
		m_Status = ParseRequest();

#ifndef NDEBUG
		DEBUG_PRINTF("%s %s", s_request_method[static_cast<uint32_t>(m_RequestMethod)], m_RequestContentType < http::contentTypes::NOT_DEFINED ? s_contentType[static_cast<uint32_t>(m_RequestContentType)] : "Unknown");
#endif

		if (m_Status == http::Status::OK) {
			// It is a supported request
			if (m_RequestMethod == http::RequestMethod::GET) {
				m_Status = HandleGet();
			} else if (m_RequestMethod == http::RequestMethod::POST) {
				// Handling headers and body data for the first chunk
				m_Status = HandlePost(false);

				if ((m_Status == http::Status::OK) && (m_nRequestDataLength == 0)) {
					DEBUG_PUTS("There is a POST header only -> no data");
					DEBUG_EXIT
					return;
				}

				if (m_nRequestDataLength < m_nRequestContentLength) {
					DEBUG_EXIT
					return;
				}
			}
#if defined (ENABLE_METHOD_DELETE)
			else if (m_RequestMethod == http::RequestMethod::DELETE) {
				m_Status = HandleDelete(false);

				if ((m_Status == http::Status::OK) && (m_nRequestDataLength == 0)) {
					DEBUG_PUTS("There is a DELETE header only -> no data");
					DEBUG_EXIT
					return;
				}
			}
#endif
		}
	} else if ((m_Status == http::Status::OK) && (m_RequestMethod == http::RequestMethod::POST)) {
		// Handling body data only for subsequent chunks
		m_Status = HandlePost(true);

		if (m_nRequestDataLength < m_nRequestContentLength) {
			DEBUG_EXIT
			return;
		}
	}
#if defined (ENABLE_METHOD_DELETE)
	else if ((m_Status == http::Status::OK) && (m_RequestMethod == http::RequestMethod::DELETE)) {
		m_Status = HandleDelete(true);
	}
#endif

	if (m_Status != http::Status::OK) {
		switch (m_Status) {
		case http::Status::BAD_REQUEST:
			pStatusMsg = "Bad Request";
			break;
		case http::Status::NOT_FOUND:
			pStatusMsg = "Not Found";
			break;
		case http::Status::REQUEST_ENTITY_TOO_LARGE:
			pStatusMsg = "Request Entity Too Large";
			break;
		case http::Status::REQUEST_URI_TOO_LONG:
			pStatusMsg = "Request-URI Too Long";
			break;
		case http::Status::INTERNAL_SERVER_ERROR:
			pStatusMsg = "Internal Server Error";
			break;
		case http::Status::METHOD_NOT_IMPLEMENTED:
			 __attribute__ ((fallthrough));
			/* no break */
		case http::Status::VERSION_NOT_SUPPORTED:
			 __attribute__ ((fallthrough));
			/* no break */
		default:
			net::tcp_abort(m_nHandle, m_nConnectionHandle);
			m_Status = http::Status::UNKNOWN_ERROR;
			m_RequestMethod = http::RequestMethod::UNKNOWN;
			DEBUG_EXIT
			return;
			break;
		}

		m_RequestContentType = http::contentTypes::TEXT_HTML;
		m_pContent = m_DynamicContent;
		m_nContentSize = static_cast<uint32_t>(snprintf(m_DynamicContent, sizeof(m_DynamicContent) - 1U,
				"<!DOCTYPE html>\n"
				"<html>\n"
				"<head><title>%u %s</title></head>\n"
				"<body><h1>%s</h1></body>\n"
				"</html>\n", static_cast<unsigned int>(m_Status), pStatusMsg, pStatusMsg));
	}

	const auto nHeaderLength = static_cast<uint32_t>(snprintf(m_pReceiveBuffer, sizeof(m_DynamicContent) - 1U,
			"HTTP/1.1 %u %s\r\n"
			"Server: %s\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %u\r\n"
			"Connection: close\r\n"
			"\r\n", static_cast<unsigned int>(m_Status), pStatusMsg, Network::Get()->GetHostName(), s_contentType[static_cast<uint32_t>(m_RequestContentType)], static_cast<unsigned int>(m_nContentSize)));

	net::tcp_write(m_nHandle, reinterpret_cast<uint8_t *>(m_pReceiveBuffer), nHeaderLength, m_nConnectionHandle);
	net::tcp_write(m_nHandle, reinterpret_cast<const uint8_t *>(m_pContent), m_nContentSize, m_nConnectionHandle);

	DEBUG_PRINTF("m_nContentLength=%u", m_nContentSize);

	m_Status = http::Status::UNKNOWN_ERROR;
	m_RequestMethod = http::RequestMethod::UNKNOWN;

	DEBUG_EXIT
}

http::Status HttpDeamonHandleRequest::ParseRequest() {
	char *pLine = m_pReceiveBuffer;
	uint32_t nLine = 0;
	auto httpStatus = http::Status::UNKNOWN_ERROR;
	m_RequestContentType = http::contentTypes::NOT_DEFINED;
	m_nRequestContentLength = 0;
	m_nRequestDataLength = 0;
	m_pFirmwareFilename = nullptr;

	for (uint32_t i = 0; i < m_nBytesReceived; i++) {
		if (m_pReceiveBuffer[i] == '\n') {
			assert(i > 1);
			m_pReceiveBuffer[i - 1] = '\0';

			if (nLine++ == 0) {
				httpStatus = ParseMethod(pLine);
			} else {
				if (pLine[0] == '\0') {
					assert((i + 1) <= m_nBytesReceived);
					m_nRequestDataLength = static_cast<uint16_t>(m_nBytesReceived - 1 - i);

					if (m_nRequestDataLength > 0) {
						m_pFileData = &m_pReceiveBuffer[i + 1];
						m_pFileData[m_nRequestDataLength] = '\0';
					}

					return http::Status::OK;
				}
				httpStatus = ParseHeaderField(pLine);
			}

			if (httpStatus != http::Status::OK) {
				return httpStatus;
			}

			pLine = &m_pReceiveBuffer[++i];
		}
	}

	return http::Status::OK;
}

/**
 * Supported: "METHOD uri HTTP/1.1"
 * Where METHOD is "GET", "POST" or "DELETE"
 */

http::Status HttpDeamonHandleRequest::ParseMethod(char *pLine) {
	assert(pLine != nullptr);
	char *pToken;

	if ((pToken = strtok(pLine, " ")) == nullptr) {
		return http::Status::METHOD_NOT_IMPLEMENTED;
	}

	if (strcmp(pToken, "GET") == 0) {
		m_RequestMethod = http::RequestMethod::GET;
	} else if (strcmp(pToken, "POST") == 0) {
		m_RequestMethod = http::RequestMethod::POST;
	}
#if defined (ENABLE_METHOD_DELETE)
	else if (strcmp(pToken, "DELETE") == 0) {
		m_RequestMethod = http::RequestMethod::DELETE;
	}
#endif
	else {
		return http::Status::METHOD_NOT_IMPLEMENTED;
	}

	if ((pToken = strtok(nullptr, " ")) == nullptr) {
		return http::Status::BAD_REQUEST;
	}

	m_pUri = pToken;

	if ((pToken = strtok(nullptr, "/")) == nullptr || strcmp(pToken, "HTTP") != 0) {
		return http::Status::BAD_REQUEST;
	}

	if ((pToken = strtok(nullptr, " \n")) == nullptr) {
		return http::Status::BAD_REQUEST;
	}

	if (strcmp(pToken, "1.1") != 0) {
		return http::Status::VERSION_NOT_SUPPORTED;
	}

	return http::Status::OK;
}

/**
 * Only interested in "Content-Type", "Content-Length", and "X-Filename"
 * Where we check for "Content-Type: application/json" and "Content-Type: application/octet-stream"
 */

http::Status HttpDeamonHandleRequest::ParseHeaderField(char *pLine) {
	DEBUG_ENTRY
	char *pToken;

	assert(pLine != nullptr);

	if ((pToken = strtok(pLine, ":")) == nullptr) {
		return http::Status::BAD_REQUEST;
	}

	DEBUG_PUTS(pToken);

	if (strcasecmp(pToken, "Content-Type") == 0) {
		if ((pToken = strtok(nullptr, " ;")) == nullptr) {
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}
		if (memcmp(pToken, "application/", 12) == 0) {
			if (strcmp(&pToken[12], "json") == 0) {
				m_RequestContentType = http::contentTypes::APPLICATION_JSON;
				DEBUG_EXIT
				return http::Status::OK;
			}

			if (strcmp(&pToken[12], "octet-stream") == 0) {
#if defined (ENABLE_FIRMWARE_UPLOAD)
				m_RequestContentType = http::contentTypes::APPLICATION_OCTET_STREAM;
				DEBUG_EXIT
				return http::Status::OK;
#else
				DEBUG_EXIT
				return http::Status::BAD_REQUEST;
#endif
			}
		}
	} else if (strcasecmp(pToken, "Content-Length") == 0) {
		if ((pToken = strtok(nullptr, " ")) == nullptr) {
			return http::Status::BAD_REQUEST;
		}

		uint32_t nTmp = 0;
		while (*pToken != '\0') {
			auto nDigit = static_cast<uint32_t>(*pToken++ - '0');
			if (nDigit > 9) {
				return http::Status::BAD_REQUEST;
			}

			nTmp *= 10;
			nTmp += nDigit;
		}

#if defined ENABLE_FIRMWARE_UPLOAD
		if (nTmp > FIRMWARE_MAX_SIZE) {
			DEBUG_EXIT
			return http::Status::REQUEST_ENTITY_TOO_LARGE;
		}
#endif

		m_nRequestContentLength = nTmp;
		DEBUG_EXIT
		return http::Status::OK;
	}
#if defined (ENABLE_FIRMWARE_UPLOAD)
	else if (strcasecmp(pToken, "X-Filename") == 0) {
		if ((pToken = strtok(nullptr, " ")) == nullptr) {
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}

		if (strncmp(pToken, firmware::FILE_NAME, firmware::FILE_NAME_LENGTH) != 0) {
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}

		m_pFirmwareFilename = pToken;
		m_pFirmwareFilename[firmware::FILE_NAME_LENGTH] = '\0';

		DEBUG_EXIT
		return http::Status::OK;
	}
#endif

	DEBUG_EXIT
	return http::Status::OK;
}

/**
 * GET
 */

http::Status HttpDeamonHandleRequest::HandleGet() {
	DEBUG_ENTRY

	uint32_t nLength = 0;
	m_pContent = &m_DynamicContent[0];

	if (memcmp(m_pUri, "/json/", 6) == 0) {
		m_RequestContentType = http::contentTypes::APPLICATION_JSON;
		const auto *pGet = &m_pUri[6];
		switch (http::get_uint(pGet)) {
		case http::json::get::LIST:
			nLength = remoteconfig::json_get_list(m_DynamicContent, sizeof(m_DynamicContent));
			break;
		case http::json::get::VERSION:
			nLength = remoteconfig::json_get_version(m_DynamicContent, sizeof(m_DynamicContent));
			break;
		case http::json::get::UPTIME:
			if (!RemoteConfig::Get()->IsEnableUptime()) {
				DEBUG_PUTS("Status::BAD_REQUEST");
				return http::Status::BAD_REQUEST;
			}
			nLength = remoteconfig::json_get_uptime(m_DynamicContent, sizeof(m_DynamicContent));
			break;
		case http::json::get::DISPLAY:
			nLength = remoteconfig::json_get_display(m_DynamicContent, sizeof(m_DynamicContent));
			break;
		case http::json::get::DIRECTORY:
			nLength = remoteconfig::json_get_directory(m_DynamicContent, sizeof(m_DynamicContent));
			break;
		case http::json::get::TIMEDATE:
			nLength = remoteconfig::timedate::json_get_timeofday(m_DynamicContent, sizeof(m_DynamicContent));
			break;
#if !defined (DISABLE_RTC)
		case http::json::get::RTCALARM:
			nLength = remoteconfig::rtc::json_get_rtc(m_DynamicContent, sizeof(m_DynamicContent));
			break;
#endif
#if defined (RDM_CONTROLLER) && !defined (CONFIG_HTTP_HTML_NO_RDM)
		case http::json::get::RDM:
			nLength = remoteconfig::rdm::json_get_rdm(m_DynamicContent, sizeof(m_DynamicContent));
			break;
#endif
#if defined (ARTNET_CONTROLLER)
		case http::json::get::POLLTABLE:
			nLength = remoteconfig::artnet::controller::json_get_polltable(m_DynamicContent, sizeof(m_DynamicContent));
			break;
#endif
#if defined (ENABLE_NET_PHYSTATUS)
		case http::json::get::PHYSTATUS:
			nLength = remoteconfig::net::json_get_phystatus(m_DynamicContent, sizeof(m_DynamicContent));
			break;
#endif
		default:
#if defined (HAVE_DMX)
			if (memcmp(pGet, "dmx/", 4) == 0) {
				const auto *pDmx = &pGet[4];
				const bool isQuestionMark = (pDmx[6] == '?'); // Handle /dmx/status?X
				if (isQuestionMark) {
					auto *p = const_cast<char *>(pDmx);
					p[6] = '\0';
				}
				DEBUG_PRINTF("pDmx=[%s]", pDmx);
				switch (http::get_uint(pDmx)) {
				case http::json::get::PORTSTATUS:
					nLength = remoteconfig::dmx::json_get_ports(m_DynamicContent, sizeof(m_DynamicContent));
					break;
				case http::json::get::STATUS: {
					const auto *pStatus = &pDmx[7];
					if (isQuestionMark && isalpha(static_cast<int>(pStatus[0])))  {
						nLength = remoteconfig::dmx::json_get_portstatus(pStatus[0], m_DynamicContent, sizeof(m_DynamicContent));
					}
				}
				default:
					break;
				}
			} else
#endif
#if defined (HAVE_PIXEL)
				if (memcmp(pGet, "pixel/", 6) == 0) {
					const auto *pPixel = &pGet[6];
					switch (http::get_uint(pPixel)) {
					case http::json::get::TYPES:
						nLength = remoteconfig::pixel::json_get_types(m_DynamicContent, sizeof(m_DynamicContent));
						break;
					case http::json::get::STATUS:
						nLength = remoteconfig::pixel::json_get_status(m_DynamicContent, sizeof(m_DynamicContent));
						break;
					default:
						break;
					}
				} else
#endif
#if defined (RDM_CONTROLLER) && !defined (CONFIG_HTTP_HTML_NO_RDM)
					if (memcmp(pGet, "rdm/", 4) == 0) {
						const auto *pRdm = &pGet[4];
						const bool isQuestionMark = (pRdm[3] == '?'); // Handle /rdm/tod?X
						if (isQuestionMark) {
							auto *p = const_cast<char *>(pRdm);
							p[3] = '\0';
						}
						switch (http::get_uint(pRdm)) {
						case http::json::get::QUEUE:
							nLength = remoteconfig::rdm::json_get_queue(m_DynamicContent, sizeof(m_DynamicContent));
							break;
						case http::json::get::PORTSTATUS:
							nLength = remoteconfig::rdm::json_get_portstatus(m_DynamicContent, sizeof(m_DynamicContent));
							break;
						case http::json::get::TOD: {
							const auto *pTod = &pRdm[4];
							if (isQuestionMark && isalpha(static_cast<int>(pTod[0])))  {
								nLength = remoteconfig::rdm::json_get_tod(pTod[0], m_DynamicContent, sizeof(m_DynamicContent));
							}
						}
						break;
						default:
							break;
						}
					} else
#endif
#if !defined(DISABLE_FS) || defined (CONFIG_USB_HOST_MSC)
						if (memcmp(pGet, "storage/", 8) == 0) {
							const auto *pStorage = &pGet[8];
							switch (http::get_uint(pStorage)) {
							case http::json::get::DIRECTORY:
								nLength = remoteconfig::storage::json_get_directory(m_DynamicContent, sizeof(m_DynamicContent));
								break;
							default:
								break;
							}
						} else
#endif
#if defined (ENABLE_PHY_SWITCH)
							if (memcmp(pGet, "dsa/", 4) == 0) {
								const auto *pDsa = &pGet[4];
								switch (http::get_uint(pDsa)) {
								case http::json::get::PORTSTATUS:
									nLength = remoteconfig::dsa::json_get_portstatus(m_DynamicContent, sizeof(m_DynamicContent));
									break;
								case http::json::get::VLANTABLE:
									nLength = remoteconfig::dsa::json_get_vlantable(m_DynamicContent, sizeof(m_DynamicContent));
									break;
								default:
									break;
								}
							} else
#endif
#if defined (NODE_SHOWFILE)
								if (memcmp(pGet, "showfile/", 9) == 0) {
									const auto *pShowfile = &pGet[9];
									switch (http::get_uint(pShowfile)) {
									case http::json::get::STATUS:
										nLength = remoteconfig::showfile::json_get_status(m_DynamicContent, sizeof(m_DynamicContent));
										break;
									case http::json::get::DIRECTORY:
										nLength = remoteconfig::showfile::json_get_directory(m_DynamicContent, sizeof(m_DynamicContent));
										break;
									default:
										break;
									}
								} else
#endif
								{
									return HandleGetTxt();

								}
			break;
		}
	}
#if defined (ENABLE_CONTENT)
	else if (strcmp(m_pUri, "/") == 0) {
		m_pContent = get_file_content("index.html", nLength, m_RequestContentType);
	}
#if defined (HAVE_DMX)
	else if (strcmp(m_pUri, "/dmx") == 0) {
		m_pContent = get_file_content("dmx.html", nLength, m_RequestContentType);
	}
#endif
#if defined (RDM_CONTROLLER) && !defined (CONFIG_HTTP_HTML_NO_RDM)
	else if (strcmp(m_pUri, "/rdm") == 0) {
		m_pContent = get_file_content("rdm.html", nLength, m_RequestContentType);
	}
#endif
#if defined (NODE_SHOWFILE)
	else if (strcmp(m_pUri, "/showfile") == 0) {
		m_pContent = get_file_content("showfile.html", nLength, m_RequestContentType);
	}
#endif
#if defined (ENABLE_PHY_SWITCH)
	else if (strcmp(m_pUri, "/dsa") == 0) {
		m_pContent = get_file_content("dsa.html", nLength, m_RequestContentType);
	}
#endif
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
	else if (strcmp(m_pUri, "/time") == 0) {
		m_pContent = get_file_content("time.html", nLength, m_RequestContentType);
	}
#endif
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
	else if (strcmp(m_pUri, "/rtc") == 0) {
		m_pContent = get_file_content("rtc.html", nLength, m_RequestContentType);
	}
#endif
	else {
		m_pContent = get_file_content(&m_pUri[1], nLength, m_RequestContentType);
	}
#endif

	if (nLength == 0) {
		DEBUG_EXIT
		return http::Status::NOT_FOUND;
	}

	m_nContentSize = nLength;

	DEBUG_EXIT
	return http::Status::OK;
}

http::Status HttpDeamonHandleRequest::HandleGetTxt() {
	auto *pFileName = &m_pUri[6];
	const auto nLength = strlen(pFileName);

	if (nLength <= 4) {
		return http::Status::BAD_REQUEST;
	}

	if (strcmp(&pFileName[nLength - 4], ".txt") != 0) {
		return http::Status::BAD_REQUEST;
	}

	const auto bIsJSON = PropertiesConfig::IsJSON();
	PropertiesConfig::EnableJSON(true);

	strncpy(m_DynamicContent, pFileName, nLength);
	m_nContentSize = RemoteConfig::Get()->HandleGet(reinterpret_cast<void *>(m_DynamicContent), sizeof(m_DynamicContent));

	PropertiesConfig::EnableJSON(bIsJSON);

	return http::Status::OK;
}

/**
 * POST
 */

http::Status HttpDeamonHandleRequest::HandlePost(const bool hasDataOnly) {
	DEBUG_ENTRY
	DEBUG_PRINTF("m_nBytesReceived=%d, m_nFileDataLength=%u, m_nRequestContentLength=%u -> hasDataOnly=%c", m_nBytesReceived, m_nRequestDataLength, m_nRequestContentLength, hasDataOnly ? 'Y' : 'N');

	if (hasDataOnly) {
		// Accumulate body data only
		m_pFileData = m_pReceiveBuffer;
		m_nRequestDataLength += m_nBytesReceived;

		// Check if we are still waiting for more data
		if (m_nRequestDataLength < m_nRequestContentLength) {
			DEBUG_PUTS("Partial body data received, waiting for more...");
#if defined (ENABLE_FIRMWARE_UPLOAD)
			if (m_RequestContentType == http::contentTypes::APPLICATION_OCTET_STREAM) {
				if (!firmware::firmware_install_continue(reinterpret_cast<const uint8_t *>(m_pFileData), m_nBytesReceived)) {
					DEBUG_EXIT
					return http::Status::BAD_REQUEST;
				}
			}
#endif
			DEBUG_EXIT
			return http::Status::OK;
		}
	} else {
		// Handle HTTP headers and content type (only in the first chunk)
		if (!((m_RequestContentType == http::contentTypes::APPLICATION_JSON) || (m_RequestContentType == http::contentTypes::APPLICATION_OCTET_STREAM))) {
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}

		if (m_RequestContentType == http::contentTypes::APPLICATION_JSON) {
			if (memcmp(m_pUri, "/json", 5) != 0) {
				DEBUG_EXIT
				return http::Status::NOT_FOUND;
			}

			m_isAction = (memcmp(m_pUri + 5, "/action", 7) == 0);
		}

#if defined (ENABLE_FIRMWARE_UPLOAD)
		if (m_RequestContentType == http::contentTypes::APPLICATION_OCTET_STREAM) {
			if (m_pFirmwareFilename == nullptr) {
				DEBUG_EXIT
				return http::Status::NOT_FOUND;
			} else {
				DEBUG_PRINTF("Firmware: Filename = %s, Size = %u", m_pFirmwareFilename, m_nRequestContentLength);
				if (!firmware::firmware_install_start(reinterpret_cast<const uint8_t *>(m_pFileData), m_nRequestDataLength)) {
					DEBUG_EXIT
					return http::Status::BAD_REQUEST;
				}
			}
		}
#endif
		// Check if full body data is received
		if (m_nRequestDataLength < m_nRequestContentLength) {
			DEBUG_PUTS("Headers processed, waiting for body data...");
			DEBUG_EXIT
			return http::Status::OK;
		}
	}

	// Process the fully received POST body

	if (m_RequestContentType == http::contentTypes::APPLICATION_JSON) {
		return HandlePostJSON();
	}

#if defined (ENABLE_FIRMWARE_UPLOAD)
	DEBUG_PRINTF("Firmware: Total bytes received = %u", m_nRequestDataLength);


	if (m_nRequestDataLength != m_nRequestContentLength) {
		m_RequestContentType = http::contentTypes::APPLICATION_JSON;
		m_pContent = &m_DynamicContent[0];
		m_nContentSize = static_cast<uint32_t>(snprintf(m_DynamicContent, sizeof(m_DynamicContent) - 1U,
				"{\"status\": \"failed\", \"message\": \"File upload not completed\"}\n"));

		DEBUG_EXIT
		return http::Status::BAD_REQUEST;
	}

	if (!firmware::firmware_install_end(reinterpret_cast<const uint8_t *>(m_pFileData), m_nBytesReceived)) {
		DEBUG_EXIT
		return http::Status::BAD_REQUEST;
	}

	m_RequestContentType = http::contentTypes::APPLICATION_JSON;
	m_pContent = &m_DynamicContent[0];
	m_nContentSize = static_cast<uint32_t>(snprintf(m_DynamicContent, sizeof(m_DynamicContent) - 1U,
			"{\"status\": \"success\", \"message\": \"File uploaded successfully\"}\n"));

	DEBUG_EXIT
	return http::Status::OK;
#endif
	DEBUG_EXIT
	return http::Status::INTERNAL_SERVER_ERROR;
}

/**
 * DELETE
 */

http::Status HttpDeamonHandleRequest::HandleDelete(const bool hasDataOnly) {
	DEBUG_PRINTF("hasDataOnly=%c -> m_nBytesReceived=%d, m_nRequestDataLength=%u, m_nRequestContentLength=%u", hasDataOnly ? 'Y' : 'N', m_nBytesReceived, m_nRequestDataLength, m_nRequestContentLength);

	if (!hasDataOnly) {
		if (m_RequestContentType != http::contentTypes::APPLICATION_JSON) {
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}

		m_isAction = (strcmp(m_pUri, "/json/action") == 0);

		if (!m_isAction) {
			DEBUG_EXIT
			return http::Status::NOT_FOUND;
		}
	}

	const auto hasHeadersOnly = (!hasDataOnly && ((m_nBytesReceived < m_nRequestContentLength) || m_nRequestDataLength == 0));

	if (hasHeadersOnly) {
		DEBUG_PUTS("hasHeadersOnly");
		DEBUG_EXIT
		return http::Status::OK;
	}

	if (hasDataOnly) {
		m_pFileData = m_pReceiveBuffer;
		m_nRequestDataLength = m_nBytesReceived;
	}

	DEBUG_PRINTF("%d|%.*s|->%d", m_nRequestDataLength, m_nRequestDataLength, m_pFileData, m_isAction);

	auto const nJsonLength = properties::convert_json_file(m_pFileData, m_nRequestDataLength, true);

	if (nJsonLength <= 0) {
		DEBUG_PUTS("Status::BAD_REQUEST");
		DEBUG_EXIT
		return http::Status::BAD_REQUEST;
	}

	m_pFileData[nJsonLength - 1] = '\0';
	debug_dump(m_pFileData, nJsonLength);

#if defined (NODE_SHOWFILE)
	if (memcmp(m_pFileData, "show=", 5) == 0) {
		remoteconfig::showfile::json_delete(m_pFileData, nJsonLength);
	} else
#endif
	{
		DEBUG_PUTS("Status::BAD_REQUEST");
		DEBUG_EXIT
		return http::Status::BAD_REQUEST;
	}

	m_RequestContentType = http::contentTypes::TEXT_HTML;
	m_pContent = &m_DynamicContent[0];
	m_nContentSize = static_cast<uint32_t>(snprintf(m_DynamicContent, sizeof(m_DynamicContent) - 1U,
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<head><title>Submit</title></head>\n"
			"<body><h1>OK</h1></body>\n"
			"</html>\n"));

	DEBUG_EXIT
	return http::Status::OK;
}

http::Status HttpDeamonHandleRequest::HandlePostJSON() {
	DEBUG_PRINTF("%d|%.*s|->%c", m_nRequestDataLength, m_nRequestDataLength, m_pFileData, m_isAction ? 'Y' : 'N');

	if (m_isAction) {
		auto const nJsonLength = properties::convert_json_file(m_pFileData, m_nRequestDataLength, true);

		if (nJsonLength <= 0) {
			DEBUG_PUTS("Status::BAD_REQUEST");
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}

		m_pFileData[nJsonLength - 1] = '\0';
		debug_dump(m_pFileData, nJsonLength);

		uint8_t value8;

		if (Sscan::Uint8(m_pFileData, "reboot", value8) == Sscan::OK) {
			if (value8 != 0) {
				if (!RemoteConfig::Get()->IsEnableReboot()) {
					DEBUG_PUTS("Status::BAD_REQUEST");
					DEBUG_EXIT
					return http::Status::BAD_REQUEST;
				}
				DEBUG_PUTS("Reboot!");
				RemoteConfig::Get()->Reboot();
				__builtin_unreachable();
			}
		} else if (Sscan::Uint8(m_pFileData, "display", value8) == Sscan::OK) {
			Display::Get()->SetSleep(value8 == 0);
			DEBUG_PRINTF("Display::Get()->SetSleep(%d)", value8 == 0);
		} else if (Sscan::Uint8(m_pFileData, "identify", value8) == Sscan::OK) {
			if (value8 != 0) {
				Hardware::Get()->SetMode(hardware::ledblink::Mode::FAST);
			} else {
				Hardware::Get()->SetMode(hardware::ledblink::Mode::NORMAL);
			}
			DEBUG_PRINTF("identify=%d", value8 != 0);
		} else if (memcmp(m_pFileData, "date=", 5) == 0) {
			remoteconfig::timedate::json_set_timeofday(m_pFileData, static_cast<uint32_t>(nJsonLength));
		}
#if !defined (DISABLE_RTC)
		else if (memcmp(m_pFileData, "rtc=", 4) == 0) {
			remoteconfig::rtc::json_set_rtc(m_pFileData, static_cast<uint32_t>(nJsonLength));
		}
#endif
#if defined (NODE_ARTNET) && defined(RDM_CONTROLLER)
		else if (Sscan::Uint8(m_pFileData, "rdm", value8) == Sscan::OK) {
			ArtNetNode::Get()->SetRdm(!(value8 != 1));
			DEBUG_PRINTF("rdm=%d", ArtNetNode::Get()->GetRdm());
		}
#endif
#if defined (NODE_SHOWFILE)
		else if (memcmp(m_pFileData, "show=", 5) == 0) {
			remoteconfig::showfile::json_set_status(m_pFileData, nJsonLength);
		}
#endif
		else {
			DEBUG_PUTS("Status::BAD_REQUEST");
			DEBUG_EXIT
			return http::Status::BAD_REQUEST;
		}
	} else {
		const auto bIsJSON = PropertiesConfig::IsJSON();

		PropertiesConfig::EnableJSON(true);
		RemoteConfig::Get()->HandleSet(m_pFileData, m_nRequestDataLength);
		PropertiesConfig::EnableJSON(bIsJSON);
	}

	m_RequestContentType = http::contentTypes::TEXT_HTML;
	m_pContent = &m_DynamicContent[0];
	m_nContentSize = static_cast<uint32_t>(snprintf(m_DynamicContent, sizeof(m_DynamicContent) - 1U,
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<head><title>Submit JSON</title></head>\n"
			"<body><h1>OK</h1></body>\n"
			"</html>\n"));

	DEBUG_EXIT
	return http::Status::OK;
}
