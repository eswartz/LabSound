
// PeakComp ported to LabSound from https://github.com/twobigears/tb_peakcomp
// The copyright on the original is reproduced here in its entirety.

/*
 The MIT License (MIT)

 Copyright (c) 2013 Two Big Ears Ltd.
 http://twobigears.com

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 Stereo L+R peak compressor with variable knee-smooth, attack, release and makeup gain.
 Varun Nair. varun@twobigears.com

 Inspired by:
 http://www.eecs.qmul.ac.uk/~josh/documents/GiannoulisMassbergReiss-dynamicrangecompression-JAES2012.pdf
 https://ccrma.stanford.edu/~jos/filters/Nonlinear_Filter_Example_Dynamic.htm
 */

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "LabSound/extended/PeakCompNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/VectorMath.h"
#include "internal/AudioBus.h"

#include <WTF/MathExtras.h>

#include <vector>

using namespace WebCore;

namespace LabSound
{

    /////////////////////////////////////////
    // Prviate PeakCompNode Implementation //
    /////////////////////////////////////////
    
    class PeakCompNode::PeakCompNodeInternal : public AudioProcessor
    {
        
    public:

        PeakCompNodeInternal(float sampleRate) : AudioProcessor(sampleRate, 2)
        {
            m_threshold = std::make_shared<AudioParam>("threshold",  0, 0, -1e6f);
            m_ratio = std::make_shared<AudioParam>("ratio",  1, 0, 10);
            m_attack = std::make_shared<AudioParam>("attack",   0.001f,  0, 1000);
            m_release = std::make_shared<AudioParam>("release", 0.001f, 0, 1000);
            m_makeup = std::make_shared<AudioParam>("makeup", 0, 0, 60);
            m_knee = std::make_shared<AudioParam>("knee", 0, 0, 1);
            
            for (int i = 0; i < 2; i++)
            {
                kneeRecursive[i] = 0.;
                attackRecursive[i] = 0.;
                releaseRecursive[i] = 0.;
            }

            // Get sample rate
            internalSampleRate = sampleRate;
            oneOverSampleRate = 1.0 / sampleRate;
        }

        virtual ~PeakCompNodeInternal() { }
        
        virtual void initialize() { }

        virtual void uninitialize() { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(ContextRenderLock& r, const WebCore::AudioBus* sourceBus, WebCore::AudioBus* destinationBus, size_t framesToProcess)
        {
            if (!numberOfChannels())
                return;
            
            // copy attributes to run time variables
            float v = m_threshold->value(r);
            if (v <= 0) {
                // dB to linear (could use the function from m_pd.h)
                threshold = powf(10, (v*0.05));
            }
            else
                threshold = 0;

            v = m_ratio->value(r);
            if (v >= 1) {
                ratio = 1./v;
            }
            else
                ratio = 1;

            v = m_attack->value(r);
            if (v >= 0.001) {
                attack = v * 0.001;
            }
            else
                attack = 0.000001;

            v = m_release->value(r);
            if (v >= 0.001) {
                release = v * 0.001;
            }
            else
                release = 0.000001;

            v = m_makeup->value(r);
            // dB to linear (could use the function from m_pd.h)
            makeupGain = pow(10, (v * 0.05));

            v = m_knee->value(r);
            if (v >= 0 && v <= 1)
            {
                // knee value (0 to 1) is scaled from 0 (hard) to 0.02 (smooth). Could be scaled to a larger number.
                knee = v * 0.02;
            }

            // calc coefficients from run time vars
            kneeCoeffs = expf(0. - (oneOverSampleRate / knee));
            kneeCoeffsMinus = 1. - kneeCoeffs;
            
            attackCoeffs = expf(0. - (oneOverSampleRate / attack));
            attackCoeffsMinus = 1. - attackCoeffs;
            
            releaseCoeff = expf(0. - (oneOverSampleRate / release));
            releaseCoeffMinus = 1. - releaseCoeff;

            // Handle both the 1 -> N and N -> N case here.
            const float * source[16];
            unsigned numChannels = numberOfChannels();
            for (unsigned int i = 0; i < numChannels; ++i)
            {
                if (sourceBus->numberOfChannels() == numChannels)
                    source[i] = sourceBus->channel(i)->data();
                else
                    source[i] = sourceBus->channel(0)->data();
            }
            
            float * dest[16];
            for (unsigned int i = 0; i < numChannels; ++i)
                dest[i] = destinationBus->channel(i)->mutableData();

            for (size_t i = 0; i < framesToProcess; ++i)
            {
                float peakEnv = 0;
                for (unsigned int j = 0; j < numChannels; ++j)
                {
                    peakEnv += source[j][i];
                }
                // Release recursive
                releaseRecursive[0] = (releaseCoeffMinus * peakEnv) + (releaseCoeff * std::max(peakEnv, float(releaseRecursive[1])));
                
                // Attack recursive
                attackRecursive[0] = ((attackCoeffsMinus * releaseRecursive[0]) + (attackCoeffs * attackRecursive[1]));
                
                // Knee smoothening and gain reduction
                kneeRecursive[0] = (kneeCoeffsMinus * std::max(std::min(((threshold + (ratio * (attackRecursive[0] - threshold))) / attackRecursive[0]), 1.), 0.)) + (kneeCoeffs * kneeRecursive[1]);

                for (unsigned int j = 0; j < numChannels; ++j)
                {
                    dest[j][i] = source[j][i] * kneeRecursive[0] * makeupGain;
                }
            }
            
            releaseRecursive[1] = releaseRecursive[0];
            attackRecursive[1] = attackRecursive[0];
            kneeRecursive[1] = kneeRecursive[0];
        }

        float internalSampleRate;
        double oneOverSampleRate;
        
        // Arrays for delay lines
        double kneeRecursive[2];
        double attackRecursive[2];
        double releaseRecursive[2];

        double attack;
        double release;
        double ratio;
        double threshold;
        
        double knee;
        double kneeCoeffs;
        double kneeCoeffsMinus;
        
        double attackCoeffs;
        double attackCoeffsMinus;
        
        double releaseCoeff;
        double releaseCoeffMinus;

        double makeupGain;

        // Resets filter state
        virtual void reset() { /* @tofix */ }

        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }

		std::shared_ptr<AudioParam> m_threshold;
		std::shared_ptr<AudioParam> m_ratio;
		std::shared_ptr<AudioParam> m_attack;
		std::shared_ptr<AudioParam> m_release;
		std::shared_ptr<AudioParam> m_makeup;
		std::shared_ptr<AudioParam> m_knee;
    };

    std::shared_ptr<AudioParam> PeakCompNode::threshold() const { return internalNode->m_threshold; }
    std::shared_ptr<AudioParam> PeakCompNode::ratio() const { return internalNode->m_ratio; }
    std::shared_ptr<AudioParam> PeakCompNode::attack() const { return internalNode->m_attack; }
    std::shared_ptr<AudioParam> PeakCompNode::release() const { return internalNode->m_release; }
    std::shared_ptr<AudioParam> PeakCompNode::makeup() const { return internalNode->m_makeup; }
    std::shared_ptr<AudioParam> PeakCompNode::knee() const { return internalNode->m_knee; }
    
    /////////////////////////
    // Public PeakCompNode //
    /////////////////////////
    
    PeakCompNode::PeakCompNode(float sampleRate) : WebCore::AudioBasicProcessorNode(sampleRate)
    {
        m_processor.reset(new PeakCompNodeInternal(sampleRate));

        internalNode = static_cast<PeakCompNodeInternal*>(m_processor.get());
        
        setNodeType((AudioNode::NodeType) LabSound::NodeTypePeakComp);

        addInput(std::unique_ptr<AudioNodeInput>(new WebCore::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo
        
        initialize();
    }
    
    PeakCompNode::~PeakCompNode()
    {
        uninitialize();
    }
    
} // End namespace LabSound
