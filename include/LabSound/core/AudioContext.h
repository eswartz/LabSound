#pragma once

// LabSound AudioContext
//
// Copyright (c) 2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef AudioContext_h
#define AudioContext_h

#include "LabSound/core/ConcurrentQueue.h"
#include "LabSound/core/AudioScheduledSourceNode.h"

#include <set>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <string>

namespace LabSound
{
	class ContextGraphLock;
	class ContextRenderLock;
}

using namespace LabSound;

namespace WebCore
{

class AudioBuffer;
class AudioDestinationNode;
class AudioListener;
class AudioNode;
class AudioScheduledSourceNode;
class HRTFDatabaseLoader;
class AudioHardwareSourceNode;
class AudioNodeInput;
class AudioNodeOutput;

template<class Input, class Output>
struct PendingConnection
{
	PendingConnection(std::shared_ptr<Input> from,
                      std::shared_ptr<Output> to,
                      bool connect) : from(from), to(to), connect(connect) {}
	bool connect; // true: connect; false: disconnect
	std::shared_ptr<Input> from;
	std::shared_ptr<Output> to;
};

//@tofix: refactor such that this factory function doesn't need to exist
std::shared_ptr<AudioHardwareSourceNode> MakeHardwareSourceNode(LabSound::ContextRenderLock & r);

class AudioContext
{
    
	friend class LabSound::ContextGraphLock;
	friend class LabSound::ContextRenderLock;
    
public:

	// It is somewhat arbitrary and could be increased if necessary.
	static const unsigned maxNumberOfChannels = 32;

    // Debugging/Sanity Checking
    std::string m_graphLocker;
    std::string m_renderLocker;

	// Realtime Context
	AudioContext();

	// Offline (non-realtime) Context
	AudioContext(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);

	~AudioContext();

	void initHRTFDatabase();

	bool isInitialized() const;

	// Returns true when initialize() was called AND all asynchronous initialization has completed.
	bool isRunnable() const;

	// Eexternal users shouldn't use this; it should be called by LabSound::init()
	// It *is* harmless to call it though, it's just not necessary.
	void lazyInitialize();

	void update(LabSound::ContextGraphLock &);

	void stop(LabSound::ContextGraphLock &);

	void setDestinationNode(std::shared_ptr<AudioDestinationNode> node);

	std::shared_ptr<AudioDestinationNode> destination();

	bool isOfflineContext();

	size_t currentSampleFrame() const;

	double currentTime() const;

	float sampleRate() const;

	AudioListener * listener();

	unsigned long activeSourceCount() const;

	void incrementActiveSourceCount();
	void decrementActiveSourceCount();

	void handlePreRenderTasks(LabSound::ContextRenderLock &); // Called at the START of each render quantum.
	void handlePostRenderTasks(LabSound::ContextRenderLock &); // Called at the END of each render quantum.

	// We schedule deletion of all marked nodes at the end of each realtime render quantum.
	void markForDeletion(LabSound::ContextRenderLock & r, AudioNode *);
	void deleteMarkedNodes();

	// AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
	// These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
	void addAutomaticPullNode(std::shared_ptr<AudioNode>);
	void removeAutomaticPullNode(std::shared_ptr<AudioNode>);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    // Only an AudioDestinationNode should call this. 
    void processAutomaticPullNodes(LabSound::ContextRenderLock &, size_t framesToProcess);
    
	// Keeps track of the number of connections made.
	void incrementConnectionCount();
	unsigned connectionCount() const 
	{ 
		return m_connectionCount; 
	}

	void connect(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to);
	void connect(std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput);

	void disconnect(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to);
	void disconnect(std::shared_ptr<AudioNode>);
	void disconnect(std::shared_ptr<AudioNodeOutput> toOutput);

	void holdSourceNodeUntilFinished(std::shared_ptr<AudioScheduledSourceNode>);
    
    // Necessary to call when using an OfflineAudioDestinationNode
    void startRendering();
    
    std::shared_ptr<AudioBuffer> getOfflineRenderTarget() { return m_renderTarget; }
	std::function<void()> offlineRenderCompleteCallback;

private:

	std::mutex m_graphLock;
	std::mutex m_renderLock;

	std::mutex automaticSourcesMutex;

	bool m_isStopScheduled = false;
	bool m_isInitialized = false;
	bool m_isAudioThreadFinished = false;
	bool m_isOfflineContext = false;
	bool m_isDeletionScheduled = false;
	bool m_automaticPullNodesNeedUpdating = false; 	// keeps track if m_automaticPullNodes is modified.

    // Number of AudioBufferSourceNodes that are active (playing).
    std::atomic<int> m_activeSourceCount;
    std::atomic<int> m_connectionCount;

	void uninitialize(LabSound::ContextGraphLock &);

	// Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
	void clear();

	void scheduleNodeDeletion(LabSound::ContextRenderLock & g);

	void referenceSourceNode(LabSound::ContextGraphLock&, std::shared_ptr<AudioNode>);
	void dereferenceSourceNode(LabSound::ContextGraphLock&, std::shared_ptr<AudioNode>);
    
	void handleAutomaticSources();
	void updateAutomaticPullNodes();

	std::shared_ptr<AudioDestinationNode> m_destinationNode;
	std::shared_ptr<AudioListener> m_listener;
	std::shared_ptr<HRTFDatabaseLoader> m_hrtfDatabaseLoader;
	std::shared_ptr<AudioBuffer> m_renderTarget;

	std::vector<std::shared_ptr<AudioNode>> m_referencedNodes;

	std::vector<std::shared_ptr<AudioNode>> m_nodesToDelete;
	std::vector<std::shared_ptr<AudioNode>> m_nodesMarkedForDeletion;

	std::set<std::shared_ptr<AudioNode>> m_automaticPullNodes; // queue for added pull nodes
	std::vector<std::shared_ptr<AudioNode>> m_renderingAutomaticPullNodes; // vector of known pull nodes

	std::vector<std::shared_ptr<AudioScheduledSourceNode>> automaticSources;

	std::vector<PendingConnection<AudioNodeInput, AudioNodeOutput>> pendingConnections;
    
    typedef PendingConnection<AudioNode, AudioNode> PendingNodeConnection;
    
    struct CompareScheduledTime
    {
        bool operator()(const PendingNodeConnection& p1, const PendingNodeConnection& p2)
        {
            if (!p2.from->isScheduledNode()) return true;
            if (!p1.from->isScheduledNode()) return false;
            AudioScheduledSourceNode * ap1 = dynamic_cast<AudioScheduledSourceNode*>(p1.from.get());
            AudioScheduledSourceNode * ap2 = dynamic_cast<AudioScheduledSourceNode*>(p2.from.get());
            return ap2->startTime() < ap1->startTime();
        }
    };
    
    std::priority_queue<PendingNodeConnection, std::deque<PendingNodeConnection>, CompareScheduledTime> pendingNodeConnections;
	//std::vector<PendingConnection<AudioNode, AudioNode>> pendingNodeConnections;
};

} // End namespace WebCore

#endif // AudioContext_h
