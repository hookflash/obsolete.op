/*

 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.

 */

#include <openpeer/stack/internal/stack_MessageMonitor.h>
#include <openpeer/stack/internal/stack_MessageMonitorManager.h>

#include <openpeer/stack/message/Message.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMessageMonitorManagerForMessageMonitor
      #pragma mark

      //-----------------------------------------------------------------------
      MessageMonitorManagerPtr IMessageMonitorManagerForMessageMonitor::singleton()
      {
        return MessageMonitorManager::singleton();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitorManager
      #pragma mark

      //-----------------------------------------------------------------------
      MessageMonitorManager::MessageMonitorManager() :
        mID(zsLib::createPUID())
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      MessageMonitorManagerPtr MessageMonitorManager::create()
      {
        MessageMonitorManagerPtr pThis(new MessageMonitorManager);
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      MessageMonitorManager::~MessageMonitorManager()
      {
        if(isNoop()) return;
        
        AutoRecursiveLock lock(mLock);
        mThisWeak.reset();
        mMonitors.clear();

        ZS_LOG_DEBUG(log("destoyed"))
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitorManager => friend MessageMonitor
      #pragma mark

      //-----------------------------------------------------------------------
      MessageMonitorManagerPtr MessageMonitorManager::singleton()
      {
        static MessageMonitorManagerPtr global = IMessageMonitorManagerFactory::singleton().createMessageMonitorManager();
        return global;
      }

      //-----------------------------------------------------------------------
      void MessageMonitorManager::monitorStart(
                                               MessageMonitorPtr monitor,
                                               const zsLib::String &requestID
                                               )
      {
        AutoRecursiveLock lock(mLock);
        mMonitors[requestID] = monitor;
      }

      //-----------------------------------------------------------------------
      void MessageMonitorManager::monitorEnd(const zsLib::String &requestID)
      {
        AutoRecursiveLock lock(mLock);
        MonitorMap::iterator found = mMonitors.find(requestID);
        if (found == mMonitors.end()) return;
        mMonitors.erase(found);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MessageMonitorManager => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String MessageMonitorManager::log(const char *message) const
      {
        return String("MessageMonitorManager [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      bool MessageMonitorManager::handleMessage(message::MessagePtr message)
      {
        AutoRecursiveLock lock(mLock);

        String id = message->messageID();
        MonitorMap::iterator found = mMonitors.find(id);
        if (found == mMonitors.end()) return false;

        MessageMonitorPtr monitor = (*found).second.lock();
        if (!monitor) {
          // the Monitor was deleted at this point so remove it
          mMonitors.erase(found);
          return false;
        }

        return monitor->forMessageMonitorManager().handleMessage(message);
      }
    }
  }
}
