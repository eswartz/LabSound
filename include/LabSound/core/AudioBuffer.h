/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioBuffer_h
#define AudioBuffer_h

#include <vector>
#include <memory>

namespace WebCore
{

class AudioBus;
class AudioBuffer;

class AudioBuffer 
{

public:   

    AudioBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);

    explicit AudioBuffer(AudioBus*);

	~AudioBuffer();

    // Format
    size_t length() const { return m_length; }
    double duration() const { return length() / sampleRate(); }
    float sampleRate() const { return m_sampleRate; }

    // Channel data access
    unsigned numberOfChannels() const { return (unsigned) m_channels.size(); }
    std::shared_ptr<std::vector<float>> getChannelData(unsigned channelIndex);
    void zero();

    // Scalar gain
    double gain() const { return m_gain; }
    void setGain(double gain) { m_gain = gain; }

    void releaseMemory();
    
protected:

    double m_gain; // scalar gain
    float m_sampleRate;
    size_t m_length;

    std::vector< std::shared_ptr<std::vector<float> > > m_channels;
};

} // namespace WebCore

#endif // AudioBuffer_h
