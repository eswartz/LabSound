/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSound/core/BiquadFilterNode.h"

#include "internal/BiquadProcessor.h"

namespace WebCore 
{

BiquadFilterNode::BiquadFilterNode(float sampleRate) : AudioBasicProcessorNode(sampleRate)
{
    // Initially setup as lowpass filter.
    m_processor.reset(new BiquadProcessor(sampleRate, 1, false));
    setNodeType(NodeTypeBiquadFilter);
    initialize();
}

void BiquadFilterNode::setType(unsigned short type)
{
    if (type > BiquadProcessor::Allpass) throw std::out_of_range("Filter type exceeds index of known types");
    
    biquadProcessor()->setType(static_cast<BiquadProcessor::FilterType>(type));
}


void BiquadFilterNode::getFrequencyResponse(ContextRenderLock& r,
                                            const std::vector<float>& frequencyHz,
                                            std::vector<float>& magResponse,
                                            std::vector<float>& phaseResponse)
{
    if (!frequencyHz.size() || !magResponse.size() || !phaseResponse.size())
        return;
    
    int n = std::min(frequencyHz.size(),
                     std::min(magResponse.size(), phaseResponse.size()));

    if (n) {
        biquadProcessor()->getFrequencyResponse(r, n, &frequencyHz[0], &magResponse[0], &phaseResponse[0]);
    }
}

BiquadProcessor * BiquadFilterNode::biquadProcessor() 
{ 
	return static_cast<BiquadProcessor*>(processor()); 
}

unsigned short BiquadFilterNode::type() 
{ 
	return biquadProcessor()->type(); 
}

std::shared_ptr<AudioParam> BiquadFilterNode::frequency() 
{ 
	return biquadProcessor()->parameter1(); 
}

std::shared_ptr<AudioParam> BiquadFilterNode::q() 
{ 
	return biquadProcessor()->parameter2(); 
}
std::shared_ptr<AudioParam> BiquadFilterNode::gain() 
{ 
	return biquadProcessor()->parameter3(); 
}

std::shared_ptr<AudioParam> BiquadFilterNode::detune() 
{ 
	return biquadProcessor()->parameter4(); 
}

} // namespace WebCore
