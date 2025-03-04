/**
 * @file json_get_queue.cpp
 *
 */
/* Copyright (C) 2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>
#include <cstdio>
#include <cassert>

#include "artnetnode.h"


namespace remoteconfig::rdm {
uint32_t json_get_queue(char *pOutBuffer, const uint32_t nOutBufferSize) {
	const auto nBufferSize = nOutBufferSize - 2U;
	auto nLength = static_cast<uint32_t>(snprintf(pOutBuffer, nBufferSize, "{\"uid\":[" ));

	nLength += ArtNetNode::Get()->RdmCopyWorkingQueue(&pOutBuffer[nLength], nBufferSize - nLength);

	pOutBuffer[nLength++] = ']';
	pOutBuffer[nLength++] = '}';

	assert(nLength <= nOutBufferSize);
	return nLength;
}
} // namespace remoteconfig::rdm



