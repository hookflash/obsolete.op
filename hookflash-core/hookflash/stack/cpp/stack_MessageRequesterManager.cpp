/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/stack/internal/stack_MessageRequester.h>
#include <hookflash/stack/internal/stack_MessageRequesterManager.h>

#include <hookflash/stack/message/Message.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

using namespace zsLib::XML;
using zsLib::PUID;
using zsLib::String;
using zsLib::ULONG;
using zsLib::UINT;
using zsLib::BYTE;
using zsLib::AutoRecursiveLock;

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      class MessageRequesterManagerGlobal
      {
      public:
        MessageRequesterManagerGlobal()
        {
          mGlobal = MessageRequesterManager::create();
        }

        static MessageRequesterManagerPtr singleton()
        {
          static MessageRequesterManagerGlobal global;
          return global.mGlobal;
        }

      private:
        MessageRequesterManagerPtr mGlobal;
      };

      static MessageRequesterManagerGlobal global;  // ensure it is created before threads are spawned

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      MessageRequesterManager::MessageRequesterManager() :
        mID(zsLib::createPUID())
      {
      }

      //-----------------------------------------------------------------------
      void MessageRequesterManager::monitorStart(
                                                 MessageRequesterPtr requester,
                                                 const zsLib::String &requestID
                                                 )
      {
        AutoRecursiveLock lock(mLock);
        mRequests[requestID] = requester;
      }

      //-----------------------------------------------------------------------
      void MessageRequesterManager::monitorEnd(const zsLib::String &requestID)
      {
        AutoRecursiveLock lock(mLock);
        RequesterMap::iterator found = mRequests.find(requestID);
        if (found == mRequests.end()) return;
        mRequests.erase(found);
      }

      //-----------------------------------------------------------------------
      MessageRequesterManager::~MessageRequesterManager()
      {
        AutoRecursiveLock lock(mLock);
        mThisWeak.reset();
        mRequests.clear();
      }

      //-----------------------------------------------------------------------
      MessageRequesterManagerPtr MessageRequesterManager::singleton()
      {
        return MessageRequesterManagerGlobal::singleton();
      }

      //-----------------------------------------------------------------------
      MessageRequesterManagerPtr MessageRequesterManager::create()
      {
        MessageRequesterManagerPtr pThis(new MessageRequesterManager);
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      bool MessageRequesterManager::handleMessage(message::MessagePtr message)
      {
        AutoRecursiveLock lock(mLock);
        String id = message->messageID();
        RequesterMap::iterator found = mRequests.find(id);
        if (found == mRequests.end()) return false;

        MessageRequesterPtr requester = (*found).second.lock();
        if (!requester) {
          // the requester was deleted at this point so remove it
          mRequests.erase(found);
          return false;
        }

        return requester->handleMessage(message);
      }
    }
  }
}
