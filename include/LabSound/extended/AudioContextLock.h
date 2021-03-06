// Copyright (c) 2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

//#define DEBUG_LOCKS

#ifndef AudioContextLock_h
#define AudioContextLock_h

#include "LabSound/core/AudioContext.h"
#include "LabSound/extended/Logging.h"

#include <iostream>
#include <mutex>

namespace LabSound
{

    class ContextGraphLock
    {
        
    public:
        
        ContextGraphLock(std::shared_ptr<WebCore::AudioContext> context, const std::string & lockSuitor)
        {
            if (context && context->m_graphLock.try_lock())
            {
                m_context = context;
                m_context->m_graphLocker = lockSuitor;
            }
#if defined(DEBUG_LOCKS)
            else if (context && context->m_graphLocker.size())
            {
                LOG("%s failed to acquire [GRAPH] lock. Currently held by: %s.", lockSuitor.c_str(), context->m_graphLocker.c_str());
            }
            else
            {
                LOG("%s failed to acquire [GRAPH] lock.", lockSuitor.c_str());
            }
#endif
        }
        
        ~ContextGraphLock()
        {
            if (m_context)
            {
                m_context->m_graphLock.unlock();
            }
            
        }
        
        WebCore::AudioContext* context() { return m_context.get(); }
        std::shared_ptr<WebCore::AudioContext> contextPtr() { return m_context; }
        
    private:
        std::shared_ptr<WebCore::AudioContext> m_context;
    };
    
    class ContextRenderLock
    {
        
    public:
        
        ContextRenderLock(std::shared_ptr<WebCore::AudioContext> context, const std::string & lockSuitor)
        {
            if (context && context->m_renderLock.try_lock())
            {
                m_context = context;
                m_context->m_renderLocker = lockSuitor;
            }
#if defined(DEBUG_LOCKS)
            else if (context && context->m_renderLocker.size())
            {
                LOG("%s failed to acquire [RENDER] lock. Currently held by: %s.", lockSuitor.c_str(), context->m_renderLocker.c_str());
            }
            else
            {
                LOG("%s failed to acquire [RENDER] lock.", lockSuitor.c_str());
            }
#endif
        }
        
        ~ContextRenderLock()
        {
            if (m_context)
                m_context->m_renderLock.unlock();
        }
        
        WebCore::AudioContext* context() { return m_context.get(); }
        std::shared_ptr<WebCore::AudioContext> contextPtr() { return m_context; }
        
    private:
        std::shared_ptr<WebCore::AudioContext> m_context;
    };

} // end namespace LabSound

#endif
