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

#include <hookflash/services/internal/services_STUNRequesterManager.h>
#include <hookflash/services/internal/services_STUNRequester.h>
#include <zsLib/Exception.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services) } }

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      static STUNRequesterManager::QWORDPair getKey(STUNPacketPtr stun)
      {
        BYTE buffer[sizeof(QWORD)*2];
        ZS_THROW_INVALID_ASSUMPTION_IF(sizeof(buffer) < (sizeof(stun->mMagicCookie) + sizeof(stun->mTransactionID)))

        memset(&(buffer[0]), 0, sizeof(buffer));

        memcpy(&(buffer[0]), &(stun->mMagicCookie), sizeof(stun->mMagicCookie));
        memcpy(&(buffer[sizeof(stun->mMagicCookie)]), &(stun->mTransactionID[0]), sizeof(stun->mTransactionID));

        QWORD q1 = 0;
        QWORD q2 = 0;
        memcpy(&q1, &(buffer[0]), sizeof(q1));
        memcpy(&q2, &(buffer[sizeof(q1)]), sizeof(q2));
        return STUNRequesterManager::QWORDPair(q1, q2);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class STUNRequesterManagerGlobal
      {
      public:
        STUNRequesterManagerGlobal()
        {
          STUNRequesterManager::singleton();
        }

        void noop()
        {
        }
      };

      // ensure the object is constructed
      static STUNRequesterManagerGlobal gSTUNRequesterManagerGlobalInit;

      //-----------------------------------------------------------------------
      static void getGlobalInit()
      {
        return gSTUNRequesterManagerGlobalInit.noop();
      }

      //-----------------------------------------------------------------------
      STUNRequesterManager::STUNRequesterManager() :
        mID(zsLib::createPUID())
      {
      }

      //-----------------------------------------------------------------------
      STUNRequesterManagerPtr STUNRequesterManager::create()
      {
        STUNRequesterManagerPtr pThis(new STUNRequesterManager);
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      STUNRequesterManagerPtr STUNRequesterManager::singleton()
      {
        static STUNRequesterManagerPtr pThis = create();
        getGlobalInit();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void STUNRequesterManager::monitorStart(
                                              STUNRequesterPtr requester,
                                              STUNPacketPtr request
                                              )
      {
        ZS_THROW_INVALID_USAGE_IF(!requester)

        QWORDPair key = getKey(request);

        AutoRecursiveLock lock(mLock);
        mRequesters[key] = STUNRequesterPair(requester, requester.get());
      }

      //-----------------------------------------------------------------------
      void STUNRequesterManager::monitorStop(STUNRequester *requester)
      {
        ZS_THROW_INVALID_USAGE_IF(!requester)

        AutoRecursiveLock lock(mLock);

        for (STUNRequesterMap::iterator iter = mRequesters.begin(); iter != mRequesters.end(); ++iter) {
          if ((*iter).second.second == requester) {
            // found the requester, remove it from the monitor map
            mRequesters.erase(iter);
            return;
          }
        }
      }

      //-----------------------------------------------------------------------
      ISTUNRequesterPtr STUNRequesterManager::handleSTUNPacket(
                                                               IPAddress fromIPAddress,
                                                               STUNPacketPtr stun
                                                               )
      {
        if ((stun->mClass == STUNPacket::Class_Request) ||
            (stun->mClass == STUNPacket::Class_Indication)) {
          ZS_LOG_TRACE(log("ignoring STUN packet that are requests or indications"))
          return ISTUNRequesterPtr();
        }

        QWORDPair key = getKey(stun);

        ZS_THROW_INVALID_USAGE_IF(!stun)

        STUNRequesterPtr requester;

        // scope: we cannot call the requester from within the lock because
        //        the requester might be calling the manager at the same
        //        time (thus trying to obtain the lock)
        {
          AutoRecursiveLock lock(mLock);
          STUNRequesterMap::iterator iter = mRequesters.find(key);
          if (iter == mRequesters.end()) {
            ZS_LOG_TRACE(log("did not find STUN requester for STUN packet"))
            stun->log(Log::Trace, "handleSTUNPacket-requester-not-found");
            return ISTUNRequesterPtr();
          }

          requester = (*iter).second.first.lock();
        }

        bool remove = false;
        if (requester) {
          // WARNING: This would be very dangerous to call within the scope
          //          of a lock - so don't do it! The requester could be
          //          calling this call to stop monitoring and this class
          //          trying to call it in return might cause a deadlock.
          //
          //          Basically, rule of thumb, do not call delegates
          //          synchronously from within the scope of a lock.
          ZS_LOG_TRACE(log("forwarding request to requester object"))
          remove = requester->handleSTUNPacket(fromIPAddress, stun);
        } else{
          ZS_LOG_TRACE(log("requester object was previously destroyed thus removing from requester manager"))
          remove = true;
        }

        if (remove) {
          AutoRecursiveLock lock(mLock);

          STUNRequesterMap::iterator iter = mRequesters.find(key);
          if (iter == mRequesters.end())
            return requester;

          mRequesters.erase(iter);
        }
        return remove ? requester : ISTUNRequesterPtr();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      String STUNRequesterManager::log(const char *message) const
      {
        return String("STUNRequesterManager [") + Stringize<PUID>(mID).string() + "] " + message;
      }
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ISTUNRequesterPtr ISTUNRequesterManager::handlePacket(
                                                          IPAddress fromIPAddress,
                                                          const BYTE *packet,
                                                          ULONG packetLengthInBytes,
                                                          STUNPacket::RFCs allowedRFCs
                                                          )
    {
      ZS_THROW_INVALID_USAGE_IF(0 == packetLengthInBytes)
      ZS_THROW_INVALID_USAGE_IF(!packet)

      STUNPacketPtr stun = STUNPacket::parseIfSTUN(packet, packetLengthInBytes, allowedRFCs, false);
      if (!stun)
        return ISTUNRequesterPtr();

      return handleSTUNPacket(fromIPAddress, stun);
    }

    //-------------------------------------------------------------------------
    ISTUNRequesterPtr ISTUNRequesterManager::handleSTUNPacket(
                                                              IPAddress fromIPAddress,
                                                              STUNPacketPtr stun
                                                              )
    {
      return internal::STUNRequesterManager::singleton()->handleSTUNPacket(fromIPAddress, stun);
    }
  }
}
