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

#include <hookflash/services/internal/services_STUNDiscovery.h>
#include <hookflash/services/ISTUNRequesterManager.h>
#include <zsLib/Exception.h>
#include <zsLib/helpers.h>

#include <algorithm>

#include <zsLib/Log.h>
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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNDiscovery
      #pragma mark

      //-----------------------------------------------------------------------
      STUNDiscovery::STUNDiscovery(
                                   IMessageQueuePtr queue,
                                   ISTUNDiscoveryDelegatePtr delegate
                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(ISTUNDiscoveryDelegateProxy::createWeak(queue, delegate))
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void STUNDiscovery::init(
                               IDNS::SRVResultPtr service,
                               const char *srvName
                               )
      {
        ZS_THROW_INVALID_USAGE_IF((!service) && (!srvName))

        AutoRecursiveLock lock(mLock);

        mSRVResult = IDNS::cloneSRV(service);
        if (!mSRVResult) {
          // attempt a DNS lookup on the name
          mSRVQuery = IDNS::lookupSRV(mThisWeak.lock(), srvName, "stun", "udp", 3478);
        }
        step();
      }

      //-----------------------------------------------------------------------
      STUNDiscovery::~STUNDiscovery()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNDiscovery => ISTUNDiscovery
      #pragma mark

      //-----------------------------------------------------------------------
      STUNDiscoveryPtr STUNDiscovery::create(
                                             IMessageQueuePtr queue,
                                             ISTUNDiscoveryDelegatePtr delegate,
                                             IDNS::SRVResultPtr service
                                             )
      {
        ZS_THROW_INVALID_USAGE_IF(!queue)
        ZS_THROW_INVALID_USAGE_IF(!delegate)
        ZS_THROW_INVALID_USAGE_IF(!service)

        STUNDiscoveryPtr pThis(new STUNDiscovery(queue, delegate));
        pThis->mThisWeak = pThis;
        pThis->init(service, NULL);
        return pThis;
      }

      //-----------------------------------------------------------------------
      STUNDiscoveryPtr STUNDiscovery::create(
                                             IMessageQueuePtr queue,
                                             ISTUNDiscoveryDelegatePtr delegate,
                                             const char *srvName
                                             )
      {
        ZS_THROW_INVALID_USAGE_IF(!queue)
        ZS_THROW_INVALID_USAGE_IF(!delegate)
        ZS_THROW_INVALID_USAGE_IF(!srvName)

        STUNDiscoveryPtr pThis(new STUNDiscovery(queue, delegate));
        pThis->mThisWeak = pThis;
        pThis->init(IDNS::SRVResultPtr(), srvName);
        return pThis;
      }

      //-----------------------------------------------------------------------
      bool STUNDiscovery::isComplete() const
      {
        AutoRecursiveLock lock(mLock);
        if (!mDelegate) return true;
        return false;
      }

      //-----------------------------------------------------------------------
      void STUNDiscovery::cancel()
      {
        AutoRecursiveLock lock(mLock);
        if (mSRVQuery) {
          mSRVQuery->cancel();
          mSRVQuery.reset();
        }
        mSRVResult.reset();
        if (mSTUNRequester) {
          mSTUNRequester->cancel();
          mSTUNRequester.reset();
        }
        mDelegate.reset();
        mPreviouslyContactedServers.clear();
      }

      //-----------------------------------------------------------------------
      IPAddress STUNDiscovery::getMappedAddress() const
      {
        AutoRecursiveLock lock(mLock);
        return mMapppedAddress;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNDiscovery => IDNSDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void STUNDiscovery::onLookupCompleted(IDNSQueryPtr query)
      {
        AutoRecursiveLock lock(mLock);
        if (query != mSRVQuery) return;

        mSRVResult = query->getSRV();
        mSRVQuery.reset();
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNDiscovery => ISTUNRequesterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void STUNDiscovery::onSTUNRequesterSendPacket(
                                                    ISTUNRequesterPtr requester,
                                                    IPAddress destination,
                                                    boost::shared_array<BYTE> packet,
                                                    ULONG packetLengthInBytes
                                                    )
      {
        AutoRecursiveLock lock(mLock);
        if (requester != mSTUNRequester) return;

        try {
          mDelegate->onSTUNDiscoverySendPacket(mThisWeak.lock(), mServer, packet, packetLengthInBytes);
        } catch(ISTUNDiscoveryDelegateProxy::Exceptions::DelegateGone &) {
          cancel(); return;
        }
      }

      //-----------------------------------------------------------------------
      bool STUNDiscovery::handleSTUNRequesterResponse(
                                                      ISTUNRequesterPtr requester,
                                                      IPAddress fromIPAddress,
                                                      STUNPacketPtr response
                                                      )
      {
        AutoRecursiveLock lock(mLock);

        if (requester != mSTUNRequester) return true;               // if true, this must be for an old request so stop trying to handle it now...

        if (response->hasAttribute(STUNPacket::Attribute_ErrorCode)) {

          // this was reporting as a successful reply but it's actually errored
          if (STUNPacket::Class_ErrorResponse != response->mClass) {
            return false; // handled the packet but its not valid so retry the request
          }

          switch (response->mErrorCode) {
            case STUNPacket::ErrorCode_TryAlternate:      {
              mServer = response->mAlternateServer;

              // make sure it has a valid port
              if (mServer.isPortEmpty()) {
                mServer.clear();
                break;
              }

              if (hasContactedServerBefore(mServer)) {
                mServer.clear();
                break;
              }

              // remember that this server has been attempted
              mPreviouslyContactedServers.push_back(mServer);
              break;
            }
            case STUNPacket::ErrorCode_BadRequest:
            case STUNPacket::ErrorCode_Unauthorized:
            case STUNPacket::ErrorCode_UnknownAttribute:
            case STUNPacket::ErrorCode_StaleNonce:
            case STUNPacket::ErrroCode_ServerError:
            default:                                      {
              // we should stop trying to contact this server
              mServer.clear();
              break;
            }
          }

          mSTUNRequester.reset();
          step();
          return true;
        }

        ZS_LOG_BASIC(log("found mapped address") + ", mapped address=" + response->mMappedAddress.string())
        mMapppedAddress = response->mMappedAddress;

        // we now have a reply, inform the delegate
        try {
          mDelegate->onSTUNDiscoveryCompleted(mThisWeak.lock());  // this is a success! yay! inform the delegate
        } catch(ISTUNDiscoveryDelegateProxy::Exceptions::DelegateGone &) {
        }

        mSTUNRequester.reset();

        cancel();
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark STUNDiscovery => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      void STUNDiscovery::onSTUNRequesterTimedOut(ISTUNRequesterPtr requester)
      {
        AutoRecursiveLock lock(mLock);
        if (requester != mSTUNRequester) return;

        // clear out the server to try so that we grab the next in the list
        mServer.clear();

        // generate a new request since we are talking to a different server
        mSTUNRequester.reset();
        step();
      }

      //-----------------------------------------------------------------------
      String STUNDiscovery::log(const char *message) const
      {
        return String("STUNDiscovery [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      void STUNDiscovery::step()
      {
        if (!mDelegate) return;                                                 // if there is no delegate then the request has completed or is cancelled
        if (mSRVQuery) return;                                                  // if an outstanding SRV lookup is being done then do nothing

        // if there is no timer then we grab extract the next SRV server to try of the SRV record
        while (mServer.isAddressEmpty()) {
          bool found = IDNS::extractNextIP(mSRVResult, mServer);
          if (!found) {
            // no more results could be found, inform of the failure...
            try {
              ZS_LOG_BASIC(log("failed to contact any STUN server"))
              mMapppedAddress.clear();

              mDelegate->onSTUNDiscoveryCompleted(mThisWeak.lock());   // sorry, nothing to report as this was a failure condition
            } catch(ISTUNDiscoveryDelegateProxy::Exceptions::DelegateGone &) {
            }
            cancel();
            return;
          }

          if (mServer.isAddressEmpty())
            continue;

          if (mServer.isPortEmpty()) {
            mServer.clear();
            continue;
          }

          if (hasContactedServerBefore(mServer)) {
            mServer.clear();
            continue;
          }

          // remember this server as being contacted before
          mPreviouslyContactedServers.push_back(mServer);
        }

        if (!mSTUNRequester) {
          // we have a server but no request, create a STUN request now...
          STUNPacketPtr request = STUNPacket::createRequest(STUNPacket::Method_Binding);
          request->mLogObject = "STUNDiscovery";
          request->mLogObjectID = mID;
          mSTUNRequester = ISTUNRequester::create(
                                                  getAssociatedMessageQueue(),
                                                  mThisWeak.lock(),
                                                  mServer,
                                                  request,
                                                  ISTUNDiscovery::usingRFC()
                                                  );
        }

        // nothing more to do... sit back, relax and enjoy the ride!
      }

      //-----------------------------------------------------------------------
      bool STUNDiscovery::hasContactedServerBefore(const IPAddress &server)
      {
        return mPreviouslyContactedServers.end() != find(mPreviouslyContactedServers.begin(), mPreviouslyContactedServers.end(), server);
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISTUNDiscovery
    #pragma mark

    //-------------------------------------------------------------------------
    ISTUNDiscoveryPtr ISTUNDiscovery::create(
                                             IMessageQueuePtr queue,
                                             ISTUNDiscoveryDelegatePtr delegate,
                                             IDNS::SRVResultPtr service
                                             )
    {
      return internal::ISTUNDiscoveryFactory::singleton().create(queue, delegate, service);
    }

    //-------------------------------------------------------------------------
    ISTUNDiscoveryPtr ISTUNDiscovery::create(
                                             IMessageQueuePtr queue,     // which message queue to use for this service (should be on the same queue as the requesting object)
                                             ISTUNDiscoveryDelegatePtr delegate,
                                             const char *srvName                // will automatically perform a stun/udp lookup on the name passed in
                                             )
    {
      return internal::ISTUNDiscoveryFactory::singleton().create(queue, delegate, srvName);
    }

    //-------------------------------------------------------------------------
    STUNPacket::RFCs ISTUNDiscovery::usingRFC()
    {
      return STUNPacket::RFC_5389_STUN;
    }

    //-------------------------------------------------------------------------
    bool ISTUNDiscovery::handleSTUNPacket(
                                          IPAddress fromIPAddress,
                                          STUNPacketPtr stun
                                          )
    {
      ISTUNRequesterPtr requester = ISTUNRequesterManager::handleSTUNPacket(fromIPAddress, stun);
      return requester;
    }

    //-------------------------------------------------------------------------
    bool ISTUNDiscovery::handlePacket(
                                      IPAddress fromIPAddress,
                                      BYTE *packet,
                                      ULONG packetLengthInBytes
                                      )
    {
      ISTUNRequesterPtr requester = ISTUNRequesterManager::handlePacket(fromIPAddress, packet, packetLengthInBytes, ISTUNDiscovery::usingRFC());
      return requester;
    }
  }
}
