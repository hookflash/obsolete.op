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

#include <openpeer/stack/internal/stack_FinderConnection.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/internal/stack_FinderRelayChannel.h>

#include <openpeer/stack/message/peer-finder/ChannelMapRequest.h>

#include <openpeer/stack/IMessageMonitor.h>

#include <openpeer/services/IHelper.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/XML.h>

#define OPENPEER_STACK_FINDER_RELAY_MULTIPLEX_OUTGOING_RECEIVE_INACTIVITY_TIMEOUT_IN_SECONDS (60*20)

#define OPENPEER_STACK_CHANNEL_MAP_REQUEST_TIMEOUT_SECONDS (60*2)

namespace openpeer { namespace stack { ZS_DECLARE_SUBSYSTEM(openpeer_stack) } }

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using services::IHelper;

      typedef ITCPMessaging::ChannelHeader ChannelHeader;
      typedef ITCPMessaging::ChannelHeaderPtr ChannelHeaderPtr;

      using peer_finder::ChannelMapRequest;
      using peer_finder::ChannelMapRequestPtr;

//      typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionManager
      #pragma mark

      class FinderConnectionManager
      {
      public:
        friend class FinderConnection;

        typedef String RemoteIPString;
        typedef std::map<RemoteIPString, FinderConnectionPtr> FinderConnectionMap;

      protected:
        //---------------------------------------------------------------------
        FinderConnectionManager()
        {
        }

        //---------------------------------------------------------------------
        void init()
        {
        }

        //---------------------------------------------------------------------
        static FinderConnectionManagerPtr create()
        {
          FinderConnectionManagerPtr pThis(new FinderConnectionManager);
          return pThis;
        }

      public:
        //---------------------------------------------------------------------
        static FinderConnectionManagerPtr singleton()
        {
          AutoRecursiveLock lock(IHelper::getGlobalLock());
          static FinderConnectionManagerPtr singleton = create();
          return singleton;
        }

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionManager => friend FinderConnection
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        FinderConnectionPtr find(const IPAddress &remoteIP)
        {
          FinderConnectionMap::iterator found = mRelays.find(remoteIP.string());
          if (found == mRelays.end()) return FinderConnectionPtr();

          return (*found).second;
        }

        //---------------------------------------------------------------------
        void add(
                 const IPAddress &remoteIP,
                 FinderConnectionPtr relay
                 )
        {
          mRelays[remoteIP.string()] = relay;
        }

        //---------------------------------------------------------------------
        void remove(const IPAddress &remoteIP)
        {
          FinderConnectionMap::iterator found = mRelays.find(remoteIP.string());
          if (found == mRelays.end()) return;

          mRelays.erase(found);
        }

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionManager => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        virtual RecursiveLock &getLock() const {return mLock;}

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionManager => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        FinderConnectionManagerWeakPtr mThisWeak;

        FinderConnectionMap mRelays;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection
      #pragma mark

      //-----------------------------------------------------------------------
      FinderConnection::FinderConnection(
                                         IMessageQueuePtr queue,
                                         IPAddress remoteFinderIP
                                         ) :
        zsLib::MessageQueueAssociator(queue),
        mCurrentState(SessionState_Pending),
        mRemoteIP(remoteFinderIP),
        mLastReceivedData(zsLib::now()),
        mLastTick(zsLib::now())
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void FinderConnection::init()
      {
        AutoRecursiveLock lock(getLock());

        mWireReceiveStream = ITransportStream::create()->getReader();
        mWireSendStream = ITransportStream::create()->getWriter();

        mTCPMessaging->connect(mThisWeak.lock(), mWireReceiveStream->getStream(), mWireSendStream->getStream(), true, mRemoteIP);

        mInactivityTimer = Timer::create(mThisWeak.lock(), Seconds(1));

        step();
      }

      //-----------------------------------------------------------------------
      FinderConnection::~FinderConnection()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      FinderConnectionPtr FinderConnection::convert(IFinderConnectionPtr connection)
      {
        return boost::dynamic_pointer_cast<FinderConnection>(connection);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => friend FinderConnectionManager
      #pragma mark

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannelPtr FinderConnection::connect(
                                                                 IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                 const IPAddress &remoteFinderIP,
                                                                 const char *localContextID,
                                                                 const char *remoteContextID,
                                                                 const char *relayDomain,
                                                                 const char *relayAccessToken,
                                                                 const char *relayAccessSecretProof,
                                                                 ITransportStreamPtr receiveStream,
                                                                 ITransportStreamPtr sendStream
                                                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)
        ZS_THROW_INVALID_ARGUMENT_IF(remoteFinderIP.isAddressEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(remoteFinderIP.isPortEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(!localContextID)
        ZS_THROW_INVALID_ARGUMENT_IF(!remoteContextID)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayDomain)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessToken)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessSecretProof)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        FinderConnectionManagerPtr manager = FinderConnectionManager::singleton();

        AutoRecursiveLock lock(manager->getLock());

        FinderConnectionPtr existing = manager->find(remoteFinderIP);

        if (existing) {
          ZS_LOG_DEBUG(existing->log("reusing existing connection"))
          return existing->connect(delegate, localContextID, remoteContextID, relayDomain, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream);
        }

        FinderConnectionPtr pThis = FinderConnection::create(remoteFinderIP);
        pThis->mThisWeak = pThis;
        pThis->mOuter = manager;
        pThis->init();

        manager->add(remoteFinderIP, pThis);

        return pThis->connect(delegate, localContextID, remoteContextID, relayDomain, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => IFinderConnection
      #pragma mark

      //-----------------------------------------------------------------------
      String FinderConnection::toDebugString(IFinderConnectionPtr connection, bool includeCommaPrefix)
      {
        if (!connection) return String(includeCommaPrefix ? ", finder connection multiplex outgoing=(null)" : "finder connection multiplex outgoing=(null)");

        FinderConnectionPtr pThis = FinderConnection::convert(connection);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      IFinderConnectionPtr FinderConnection::connect(
                                                     IFinderConnectionDelegatePtr delegate,
                                                     const IPAddress &remoteFinderIP,
                                                     ITransportStreamPtr receiveStream,
                                                     ITransportStreamPtr sendStream
                                                     )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        FinderConnectionPtr pThis = FinderConnection::create(remoteFinderIP);
        pThis->mThisWeak = pThis;

        if (delegate) {
          pThis->mDefaultSubscription = pThis->subscribe(delegate);
        }

        pThis->init();

        ChannelPtr channel = Channel::incoming(pThis, pThis, receiveStream, sendStream, 0);
        pThis->mChannels[0] = channel;

        return pThis;
      }

      //-----------------------------------------------------------------------
      IFinderConnectionSubscriptionPtr FinderConnection::subscribe(IFinderConnectionDelegatePtr originalDelegate)
      {
        ZS_LOG_TRACE(log("subscribe called"))

        AutoRecursiveLock lock(getLock());

        if (!originalDelegate) return mDefaultSubscription;

        IFinderConnectionSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate, IStackForInternal::queueDelegate());

        IFinderConnectionDelegatePtr delegate = mSubscriptions.delegate(subscription);

        if (delegate) {
          FinderConnectionPtr pThis(mThisWeak.lock());

          if (SessionState_Pending != mCurrentState) {
            delegate->onFinderConnectionStateChanged(pThis, mCurrentState);
          }

          if (isShutdown()) {
            ZS_LOG_WARNING(Detail, log("subscription created after shutdown"))
            return subscription;
          }

          for (ChannelMap::iterator iter = mIncomingChannels.begin(); iter != mIncomingChannels.end(); ++iter)
          {
            delegate->onFinderConnectionIncomingRelayChannel(pThis);
          }
        }
        
        return subscription;
      }

      //-----------------------------------------------------------------------
      void FinderConnection::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        setState(SessionState_Shutdown);

        mThisWeak.reset();

        mSubscriptions.clear();
        mDefaultSubscription->cancel();

        FinderConnectionManagerPtr outer = mOuter.lock();
        if (outer) {
          outer->remove(mRemoteIP);
        }

        if (mTCPMessaging) {
          mTCPMessaging->shutdown();
          mTCPMessaging.reset();
        }

        mWireReceiveStream->cancel();
        mWireSendStream->cancel();

        if (mInactivityTimer) {
          mInactivityTimer->cancel();
          mInactivityTimer.reset();
        }

        // scope: clear out channels
        {
          for (ChannelMap::iterator iter = mChannels.begin(); iter != mChannels.end(); ++iter)
          {
            ChannelPtr channel = (*iter).second;
            channel->cancel();
          }
          mChannels.clear();
        }

        mPendingMapRequest.clear();
        mIncomingChannels.clear();
        mRemoveChannels.clear();

        if (mMapRequestChannelMonitor) {
          mMapRequestChannelMonitor->cancel();
          mMapRequestChannelMonitor.reset();
        }
      }

      //-----------------------------------------------------------------------
      FinderConnection::SessionStates FinderConnection::getState(
                                                                 WORD *outLastErrorCode,
                                                                 String *outLastErrorReason
                                                                 ) const
      {
        AutoRecursiveLock lock(getLock());
        if (outLastErrorCode) *outLastErrorCode = mLastError;
        if (outLastErrorReason) *outLastErrorReason = mLastErrorReason;
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      IFinderRelayChannelPtr FinderConnection::accept(
                                                      IFinderRelayChannelDelegatePtr delegate,
                                                      AccountPtr account,
                                                      ITransportStreamPtr receiveStream,
                                                      ITransportStreamPtr sendStream
                                                      )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        ZS_LOG_DEBUG(log("accept called"))

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot accept channel as already shutdown"))
          return IFinderRelayChannelPtr();
        }

        stepCleanRemoval();

        if (mIncomingChannels.size() < 1) {
          ZS_LOG_WARNING(Detail, log("no pending channels found"))
          return IFinderRelayChannelPtr();
        }

        ChannelMap::iterator found = mIncomingChannels.begin();

        ChannelNumber channelNumber = (*found).first;
        ChannelPtr channel = (*found).second;

        ZS_LOG_DEBUG(log("accepting channel") + ", channel number=" + string(channelNumber))

        ITransportStreamPtr wireReceiveStream;
        ITransportStreamPtr wireSendStream;

        channel->getStreams(wireReceiveStream, wireSendStream);

        FinderRelayChannelPtr relay = IFinderRelayChannelForFinderConnection::createIncoming(delegate, account, receiveStream, sendStream, wireReceiveStream, wireSendStream);

        mIncomingChannels.erase(found);

        if (mSendStreamNotifiedReady) {
          ZS_LOG_DEBUG(log("notify channel that it's now write ready"))
          channel->notifyReceivedWireWriteReady();
        }

        return relay;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::onTimer(TimerPtr timer)
      {
        AutoRecursiveLock lock(getLock());

        if (timer != mInactivityTimer) {
          ZS_LOG_WARNING(Detail, log("notified about an obsolete timer"))
          return;
        }

        Time lastTick = mLastTick;
        Time tick = zsLib::now();
        mLastTick = tick;

        if (tick + Seconds(5) < lastTick) {
          ZS_LOG_WARNING(Detail, log("timer was not firing likely because application went to sleep (re-adjusting timer)"))

          // fake that we received data just so the inactivity timer waits a bit longer before disconnecting in a wake-up situation
          mLastReceivedData = tick;
          return;
        }

        if (tick + Seconds(OPENPEER_STACK_FINDER_RELAY_MULTIPLEX_OUTGOING_RECEIVE_INACTIVITY_TIMEOUT_IN_SECONDS) < mLastReceivedData) {
          // inactivity timeout
          setError(IHTTP::HTTPStatusCode_RequestTimeout, "finder relay channel inactivity time-out");
          cancel();
          return;
        }
      }

      //-----------------------------------------------------------------------
      void FinderConnection::onWake()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("on wake"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => ITCPMessagingDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::onTCPMessagingStateChanged(
                                                        ITCPMessagingPtr messaging,
                                                        ITCPMessaging::SessionStates state
                                                        )
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("on transport stream write ready"))

        if (writer == mWireSendStream) {
          get(mSendStreamNotifiedReady) = true;

          for (ChannelMap::iterator iter = mChannels.begin(); iter != mChannels.end(); ++iter)
          {
            ChannelNumber channelNumber = (*iter).first;
            ChannelPtr &channel = (*iter).second;

            ChannelMap::iterator found = mPendingMapRequest.find(channelNumber);
            if (found != mPendingMapRequest.end()) {
              ZS_LOG_DEBUG(log("cannot notify about write ready because channel map request has not completed yet") + ", channel number=" + string(channelNumber))
              continue;
            }
            channel->notifyReceivedWireWriteReady();
          }
        }

        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => ITransportStreamReaderDelegate
      #pragma mark

      void FinderConnection::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("on transport stream read ready"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => IFinderConnectionRelayChannelDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::onFinderRelayChannelTCPOutgoingStateChanged(
                                                                         IFinderConnectionRelayChannelPtr channel,
                                                                         IFinderConnectionRelayChannel::SessionStates state
                                                                         )
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => IMessageMonitorResultDelegate<ChannelMapResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool FinderConnection::handleMessageMonitorResultReceived(
                                                                IMessageMonitorPtr monitor,
                                                                ChannelMapResultPtr result
                                                                )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mMapRequestChannelMonitor) {
          ZS_LOG_WARNING(Detail, log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_DEBUG(log("channel map request completed successfully") + ", channel number=" + string(mMapRequestChannelNumber))

        mMapRequestChannelMonitor.reset();

        // scope: remove from pending list
        {
          ChannelMap::iterator found = mPendingMapRequest.find(mMapRequestChannelNumber);
          if (found != mPendingMapRequest.end()) {
            ZS_LOG_TRACE(log("removed from pending map"))
            mPendingMapRequest.erase(found);
          }
        }

        get(mMapRequestChannelNumber) = 0;

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();

        return true;
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::handleMessageMonitorErrorResultReceived(
                                                                     IMessageMonitorPtr monitor,
                                                                     ChannelMapResultPtr ignore, // will always be NULL
                                                                     message::MessageResultPtr result
                                                                     )
      {
        AutoRecursiveLock lock(getLock());
        if (monitor != mMapRequestChannelMonitor) {
          ZS_LOG_WARNING(Detail, log("notified about obsolete monitor"))
          return false;
        }

        ZS_LOG_WARNING(Detail, log("channel map request failed") + ", channel number=" + string(mMapRequestChannelNumber))

        mMapRequestChannelMonitor.reset();

        // scope: remove from channels list
        {
          ChannelMap::iterator found = mChannels.find(mMapRequestChannelNumber);
          if (found != mChannels.end()) {
            ZS_LOG_TRACE(log("removed from channels map"))
            ChannelPtr channel = (*found).second;
            ZS_THROW_BAD_STATE_IF(!channel)
            channel->cancel();
            mChannels.erase(found);
          }
        }

        // scope: remove from pending list
        {
          ChannelMap::iterator found = mPendingMapRequest.find(mMapRequestChannelNumber);
          if (found != mPendingMapRequest.end()) {
            ZS_LOG_TRACE(log("removed from pending map"))
            mPendingMapRequest.erase(found);
          }
        }

        get(mMapRequestChannelNumber) = 0;

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();

        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => friend ChannelOutgoing
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::sendBuffer(
                                        ChannelNumber channelNumber,
                                        SecureByteBlockPtr buffer
                                        )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!buffer)

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot send data while shutdown"))
          return;
        }

        ZS_LOG_DEBUG(log("send buffer called") + ", channel number=" + string(channelNumber) + ", buffer size=" + string(buffer->SizeInBytes()))

        ChannelHeaderPtr header(new ChannelHeader);
        header->mChannelID = channelNumber;

        mWireSendStream->write(buffer, header);
      }

      //-----------------------------------------------------------------------
      void FinderConnection::notifyDestroyed(ChannelNumber channelNumber)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("channel is destroyed") + ", channel number=" + string(channelNumber))

        if (isShutdown()) {
          ZS_LOG_WARNING(Trace, log("finder connection already destroyed (probably okay)"))
          return;
        }

        mRemoveChannels[channelNumber] = ChannelPtr();
        
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection => (internal)
      #pragma mark


      //-----------------------------------------------------------------------
      RecursiveLock &FinderConnection::getLock() const
      {
        FinderConnectionManagerPtr outer = mOuter.lock();
        if (!outer) return mLocalLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String FinderConnection::log(const char *message) const
      {
        return String("FinderConnection [" + string(mID) + "] " + message);
      }

      //-----------------------------------------------------------------------
      String FinderConnection::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;

        return
        Helper::getDebugValue("finder connection multiplex id", string(mID), firstTime) +
        Helper::getDebugValue("outer", mOuter.lock() ? String("true") : String(), firstTime) +
        Helper::getDebugValue("subscriptions", mSubscriptions.size() > 0 ? string(mSubscriptions.size()) : String(), firstTime) +
        Helper::getDebugValue("default subscription", mDefaultSubscription ? String("true") : String(), firstTime) +
        Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
        Helper::getDebugValue("last error", 0 != mLastError ? string(mLastError) : String(), firstTime) +
        Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +
        Helper::getDebugValue("remote ip", mRemoteIP.string(), firstTime) +
        ", tcp messaging: " + ITCPMessaging::toDebugString(mTCPMessaging, false) +
        ", wire recv stream: " + ITransportStream::toDebugString(mWireReceiveStream->getStream(), false) +
        ", wire send stream: " + ITransportStream::toDebugString(mWireSendStream->getStream(), false) +
        Helper::getDebugValue("send stream notified ready", mSendStreamNotifiedReady ? String("true") : String(), firstTime) +
        Helper::getDebugValue("last tick", Time() != mLastTick ? IHelper::timeToString(mLastTick) : String(), firstTime) +
        Helper::getDebugValue("last received data", Time() != mLastReceivedData ? IHelper::timeToString(mLastReceivedData) : String(), firstTime) +
        Helper::getDebugValue("inactivity timer", mInactivityTimer ? String("true") : String(), firstTime) +
        Helper::getDebugValue("channels", mChannels.size() > 0 ? string(mChannels.size()) : String(), firstTime) +
        Helper::getDebugValue("pending map request channels", mPendingMapRequest.size() > 0 ? string(mPendingMapRequest.size()) : String(), firstTime) +
        Helper::getDebugValue("incoming channels", mIncomingChannels.size() > 0 ? string(mIncomingChannels.size()) : String(), firstTime) +
        Helper::getDebugValue("remove channels", mRemoveChannels.size() > 0 ? string(mRemoveChannels.size()) : String(), firstTime) +
        Helper::getDebugValue("map request monitor", mMapRequestChannelMonitor ? String("true") : String(), firstTime) +
        Helper::getDebugValue("map request channel number", mMapRequestChannelNumber != 0 ? string(mMapRequestChannelNumber) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void FinderConnection::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState))
        mCurrentState = state;
        FinderConnectionPtr pThis = mThisWeak.lock();

        if (pThis) {
          mSubscriptions.delegate()->onFinderConnectionStateChanged(pThis, mCurrentState);
        }
      }

      //-----------------------------------------------------------------------
      void FinderConnection::setError(WORD errorCode, const char *inReason)
      {
        String reason(inReason ? String(inReason) : String());
        if (reason.isEmpty()) {
          reason = IHTTP::toString(IHTTP::toStatusCode(errorCode));
        }

        if (0 != mLastError) {
          ZS_LOG_WARNING(Detail, log("error already set thus ignoring new error") + ", new error=" + string(errorCode) + ", new reason=" + reason + getDebugValueString())
          return;
        }

        get(mLastError) = errorCode;
        mLastErrorReason = reason;

        ZS_LOG_WARNING(Detail, log("error set") + ", code=" + string(mLastError) + ", reason=" + mLastErrorReason + getDebugValueString())
      }

      //-----------------------------------------------------------------------
      void FinderConnection::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step continue to shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!stepCleanRemoval()) return;
        if (!stepChannelMapRequest()) return;

        setState(SessionState_Connected);
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::stepCleanRemoval()
      {
        if (mRemoveChannels.size() < 1) {
          ZS_LOG_DEBUG(log("no channels to remove"))
          return true;
        }

        for (ChannelMap::iterator iter = mRemoveChannels.begin(); iter != mRemoveChannels.end(); ++iter) {
          ChannelNumber channelNumber = (*iter).first;

          ZS_LOG_DEBUG(log("removing channel") + ", channel number=" + string(channelNumber))

          bool foundInPendingMapRequest = false;

          // scope: remove from pending map request
          {
            ChannelMap::iterator found = mPendingMapRequest.find(channelNumber);
            if (found != mPendingMapRequest.end()) {
              ZS_LOG_DEBUG(log("removing channel from pending map request"))
              foundInPendingMapRequest = true;
              mPendingMapRequest.erase(found);
            }
          }

          if ((mMapRequestChannelMonitor) &&
              (mMapRequestChannelNumber == channelNumber)) {
            ZS_LOG_WARNING(Detail, log("outstanding channel map request is being cancelled for channel"))
            mMapRequestChannelMonitor->cancel();
            mMapRequestChannelMonitor.reset();
          }

          // scope: remove from incoming channels
          {
            ChannelMap::iterator found = mIncomingChannels.find(channelNumber);
            if (found != mPendingMapRequest.end()) {
              ZS_LOG_DEBUG(log("removing channel from incoming channels"))
              mIncomingChannels.erase(found);
            }
          }

          // scope: remove from incoming channels
          {
            ChannelMap::iterator found = mChannels.find(channelNumber);
            if (found != mChannels.end()) {
              ZS_LOG_DEBUG(log("removing channel from incoming channels"))

              ChannelPtr channel = (*found).second;

              ZS_THROW_BAD_STATE_IF(!channel)

              channel->cancel();

              if (0 == channelNumber) {
                // in this special case, everything must shutdown...
                WORD errorCode = 0;
                String errorReason;
                channel->getState(&errorCode, &errorReason);

                ZS_LOG_DEBUG(log("master relay channel is shutdown (so must now self destruct)") + ", error code=" + string(errorCode) + ", reason=" + errorReason)

                setError(errorCode, errorReason);
                cancel();
                return false;
              }

              mChannels.erase(found);
            }
          }

          if (!foundInPendingMapRequest) {
            // notify remote party of channel closure
            ChannelHeaderPtr header(new ChannelHeader);
            header->mChannelID = channelNumber;

            SecureByteBlockPtr buffer(new SecureByteBlock);

            // by writing a buffer of "0" size to the channel number, it will cause the channel to close
            mWireSendStream->write(buffer, header);
          }

          ZS_LOG_DEBUG(log("remove channels completed"))

          mRemoveChannels.clear();
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::stepConnectWire()
      {
        WORD error = 0;
        String reason;
        switch (mTCPMessaging->getState(&error, &reason))
        {
          case ITCPMessaging::SessionState_Pending:
          {
            ZS_LOG_DEBUG(log("waiting for TCP messaging to connect"))
            return false;
          }
          case ITCPMessaging::SessionState_Connected:
          {
            ZS_LOG_DEBUG(log("TCP messaging connected"))
            break;
          }
          case ITCPMessaging::SessionState_ShuttingDown:
          case ITCPMessaging::SessionState_Shutdown:      {
            ZS_LOG_WARNING(Detail, log("TCP messaging is shutting down") + ", error=" + string(error) + ", reason=" + reason)
            if (0 != error) {
              setError(error, reason);
            }
            cancel();
            return false;
          }
        }

        if (!mSendStreamNotifiedReady) {
          ZS_LOG_DEBUG(log("waiting for TCP messaging send stream to be notified as ready"))
          return false;
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::stepChannelMapRequest()
      {
        if (mMapRequestChannelMonitor) {
          ZS_LOG_DEBUG(log("pending channel map request is already outstanding"))
          return true;
        }
        if (mPendingMapRequest.size() < 1) {
          ZS_LOG_DEBUG(log("no pending channels needing to be notified"))
          return true;
        }

        ChannelMap::iterator found = mPendingMapRequest.begin();
        ZS_THROW_BAD_STATE_IF(found == mPendingMapRequest.end())

        ChannelNumber channelNumber = (*found).first;
        ChannelPtr channel = (*found).second;

        ZS_LOG_DEBUG(log("sending channel map request") + ", channel=" + string(channelNumber))

        const Channel::ConnectionInfo &info = channel->getConnectionInfo();

        ChannelMapRequestPtr request = ChannelMapRequest::create();
        request->domain(info.mRelayDomain);
        request->channelNumber(channelNumber);
        request->localContextID(info.mLocalContextID);
        request->remoteContextID(info.mRemoteContextID);
        request->relayAccessToken(info.mRelayAccessToken);
        request->relayAccessSecretProof(info.mRelayAccessSecretProof);

        mMapRequestChannelMonitor = IMessageMonitor::monitor(IMessageMonitorResultDelegate<ChannelMapResult>::convert(mThisWeak.lock()), request, Seconds(OPENPEER_STACK_CHANNEL_MAP_REQUEST_TIMEOUT_SECONDS));
        get(mMapRequestChannelNumber) = channelNumber;

        DocumentPtr doc = request->encode();

        ULONG outputLength = 0;
        GeneratorPtr generator = Generator::createJSONGenerator();
        boost::shared_array<char> output = generator->write(doc, &outputLength);

        ChannelHeaderPtr header(new ChannelHeader);
        header->mChannelID = channelNumber;

        mWireSendStream->write((const BYTE *) (output.get()), outputLength, header);

        mPendingMapRequest.erase(found);

        return true;
      }

      //-----------------------------------------------------------------------
      FinderConnectionPtr FinderConnection::create(IPAddress remoteFinderIP)
      {
        //TODO
        return FinderConnectionPtr();
      }

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannelPtr FinderConnection::connect(
                                                                 IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                 const char *localContextID,
                                                                 const char *remoteContextID,
                                                                 const char *relayDomain,
                                                                 const char *relayAccessToken,
                                                                 const char *relayAccessSecretProof,
                                                                 ITransportStreamPtr receiveStream,
                                                                 ITransportStreamPtr sendStream
                                                                 )
      {
        //TODO
        return IFinderConnectionRelayChannelPtr();
      }








      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel
      #pragma mark

      FinderConnection::Channel::Channel(
                                         IMessageQueuePtr queue,
                                         IFinderConnectionRelayChannelDelegatePtr delegate,
                                         ITransportStreamPtr receiveStream,
                                         ITransportStreamPtr sendStream,
                                         ChannelNumber channelNumber
                                         ) :
        MessageQueueAssociator(queue)
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::init()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      FinderConnection::Channel::~Channel()
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      FinderConnection::ChannelPtr FinderConnection::Channel::convert(IFinderConnectionRelayChannelPtr channel)
      {
        //TODO
        return ChannelPtr();
      }

      //-----------------------------------------------------------------------
      String FinderConnection::Channel::toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix)
      {
        //TOOD
        return String();
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel => IFinderConnectionRelayChannel
      #pragma mark

      //-----------------------------------------------------------------------
      FinderConnection::ChannelPtr FinderConnection::Channel::connect(
                                                                      FinderConnectionPtr outer,
                                                                      IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                      const char *localContextID,
                                                                      const char *remoteContextID,
                                                                      const char *relayDomain,
                                                                      const char *relayAccessToken,
                                                                      const char *relayAccessSecretProof,
                                                                      ITransportStreamPtr receiveStream,
                                                                      ITransportStreamPtr sendStream,
                                                                      ULONG channelNumber
                                                                      )
      {
        //TODO
        return ChannelPtr();
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::cancel()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannel::SessionStates FinderConnection::Channel::getState(
                                                                                                        WORD *outLastErrorCode,
                                                                                                        String *outLastErrorReason
                                                                                                        ) const
      {
        AutoRecursiveLock lock(getLock());
        //TODO
        return IFinderConnectionRelayChannel::SessionState_Pending;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel => ITransportStreamReaderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel => friend FinderConnection
      #pragma mark

      FinderConnection::ChannelPtr FinderConnection::Channel::incoming(
                                                                                                         FinderConnectionPtr outer,
                                                                                                         IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                                                         ITransportStreamPtr receiveStream,
                                                                                                         ITransportStreamPtr sendStream,
                                                                                                         ULONG channelNumber
                                                                                                         )
      {
        //TODO
        return ChannelPtr();
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::notifyReceivedWireWriteReady()
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::notifyDataReceived(SecureByteBlockPtr buffer)
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::getStreams(
                                                                  ITransportStreamPtr &outReceiveStream,
                                                                  ITransportStreamPtr &outSendStream
                                                                  )
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &FinderConnection::Channel::getLock() const
      {
        FinderConnectionPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }
      
      //-----------------------------------------------------------------------
      String FinderConnection::Channel::log(const char *message) const
      {
        //TODO
        return String();
      }

      //-----------------------------------------------------------------------
      String FinderConnection::Channel::getDebugValueString(bool includeCommaPrefix) const
      {
        //TODO
        return String();
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::setState(SessionStates state)
      {
        //TODO
      }
      
      //-----------------------------------------------------------------------
      void FinderConnection::Channel::setError(WORD errorCode, const char *inReason)
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderConnection
      #pragma mark

      //-----------------------------------------------------------------------
      const char *IFinderConnection::toString(SessionStates state)
      {
        switch (state)
        {
          case SessionState_Pending:      return "Pending";
          case SessionState_Connected:    return "Connected";
          case SessionState_Shutdown:     return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      IFinderConnectionPtr IFinderConnection::connect(
                                                      IFinderConnectionDelegatePtr delegate,
                                                      const IPAddress &remoteFinderIP,
                                                      ITransportStreamPtr receiveStream,
                                                      ITransportStreamPtr sendStream
                                                      )
      {
        return internal::IFinderConnectionRelayChannelFactory::singleton().connect(delegate, remoteFinderIP, receiveStream, sendStream);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderConnectionRelayChannel
      #pragma mark

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannelPtr IFinderConnectionRelayChannel::connect(
                                                                              IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                              const IPAddress &remoteFinderIP,
                                                                              const char *localContextID,
                                                                              const char *remoteContextID,
                                                                              const char *relayDomain,
                                                                              const char *relayAccessToken,
                                                                              const char *relayAccessSecretProof,
                                                                              ITransportStreamPtr receiveStream,
                                                                              ITransportStreamPtr sendStream
                                                                              )
      {
        return internal::IFinderConnectionRelayChannelFactory::singleton().connect(delegate, remoteFinderIP, localContextID, remoteContextID, relayDomain, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream);
      }
    }
  }
}
