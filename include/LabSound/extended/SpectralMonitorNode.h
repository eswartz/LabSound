// Copyright (c) 2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AudioContext.h"

#include <vector>
#include <mutex>

namespace LabSound 
{
    class SpectralMonitorNode : public WebCore::AudioBasicInspectorNode 
	{
		class SpectralMonitorNodeInternal;
        std::unique_ptr<SpectralMonitorNodeInternal> internalNode;
    public:

        SpectralMonitorNode(float sampleRate);
        virtual ~SpectralMonitorNode();

        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        void spectralMag(std::vector<float>& result);
        void windowSize(size_t ws);
        size_t windowSize() const;

    private:

        // required for BasicInspector
        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }
    };
} 
