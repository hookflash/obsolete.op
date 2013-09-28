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

      typedef ITransportStream::StreamHeader StreamHeader;
      typedef ITransportStream::StreamHeaderPtr StreamHeaderPtr;

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

        mWireReceiveStream = ITransportStream::create(ITransportStreamWriterDelegatePtr(), mThisWeak.lock())->getReader();
        mWireSendStream = ITransportStream::create(mThisWeak.lock(), ITransportStreamReaderDelegatePtr())->getWriter();

        mTCPMessaging = ITCPMessaging::connect(mThisWeak.lock(), mWireReceiveStream->getStream(), mWireSendStream->getStream(), true, mRemoteIP);

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

        FinderConnectionPtr pThis(new FinderConnection(IStackForInternal::queueStack(), remoteFinderIP));
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
        if (!connection) return String(includeCommaPrefix ? ", finder connection=(null)" : "finder connection=(null)");

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

        FinderConnectionPtr pThis(new FinderConnection(IStackForInternal::queueStack(), remoteFinderIP));
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
                                                      ITransportStreamPtr sendStream,
                                                      ChannelNumber *outChannelNumber
                                                      )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!account)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        if (outChannelNumber) {
          *outChannelNumber = 0;
        }

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

        if (outChannelNumber) {
          *outChannelNumber = channelNumber;
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
        if (ITCPMessaging::SessionState_Connected == state) {
          ZS_LOG_TRACE(log("enabling TCP keep-alive"))
          messaging->enableKeepAlive();
        }
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

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return;
        }

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

      //-----------------------------------------------------------------------
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
      void FinderConnection::onFinderConnectionRelayChannelStateChanged(
                                                                        IFinderConnectionRelayChannelPtr channel,
                                                                        IFinderConnectionRelayChannel::SessionStates state
                                                                        )
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("notified master relay channel state changed"))
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

            if (mSendStreamNotifiedReady) {
              ChannelPtr channel = (*found).second;
              ZS_THROW_BAD_STATE_IF(!channel)
              ZS_LOG_DEBUG(log("notifying channel of write ready"))
              channel->notifyReceivedWireWriteReady();
            }
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
      void FinderConnection::notifyOuterWriterReady()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("notify outer writer ready"))

        if (mWireReceiveStream) {
          mWireReceiveStream->notifyReaderReadyToRead();
        }
      }

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
      bool FinderConnection::isFinderSessionConnection() const
      {
        return !(mOuter.lock());  // if points to outer, this is a relay-only channel
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::isFinderRelayConnection() const
      {
        return !isFinderSessionConnection();
      }

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
        Helper::getDebugValue("finder connection id", string(mID), firstTime) +
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

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState) + getDebugValueString())

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
        if (!stepConnectWire()) return;
        if (!stepMasterChannel()) return;
        if (!stepChannelMapRequest()) return;
        if (!stepReceiveData()) return;

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
      bool FinderConnection::stepMasterChannel()
      {
        if (isFinderRelayConnection()) {
          ZS_LOG_DEBUG(log("finder relay connections do not have a master channel"))
          return true;
        }

        ChannelMap::iterator found = mChannels.find(0);
        if (found == mChannels.end()) {
          ZS_LOG_WARNING(Detail, log("could not find master channel thus must shutdown"))
          setError(IHTTP::HTTPStatusCode_NotFound, "could not find master channel");
          cancel();
          return false;
        }

        const ChannelPtr &channel = (*found).second;

        WORD error = 0;
        String reason;

        switch (channel->getState(&error, &reason)) {
          case IFinderConnectionRelayChannel::SessionState_Pending:   {
            ZS_LOG_DEBUG(log("waiting for master channel to connect"))
            return false;
          }
          case IFinderConnectionRelayChannel::SessionState_Connected: break;
          case IFinderConnectionRelayChannel::SessionState_Shutdown:  {
            ZS_LOG_WARNING(Detail, log("finder channel is shutdown thus finder connection must shutdown"))
            setError(error, reason);
            return false;
          }
        }

        ZS_LOG_DEBUG(log("master channel connected"))
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
        header->mChannelID = 0;

        mWireSendStream->write((const BYTE *) (output.get()), outputLength, header);

        if (ZS_IS_LOGGING(Debug)) {
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log(") ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) )"))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log("CHANNEL MESSAGE") + "=" + "\n" + ((CSTR)(output.get())) + "\n")
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
          ZS_LOG_DETAIL(log(") ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) ) )"))
          ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

          ZS_LOG_DETAIL(log("v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v"))
          ZS_LOG_DETAIL(log("||| MESSAGE INFO |||") + Message::toDebugString(request))
          ZS_LOG_DETAIL(log("^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^"))
        }

        mPendingMapRequest.erase(found);

        return true;
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::stepReceiveData()
      {
        if (mWireReceiveStream->getTotalReadBuffersAvailable() < 1) {
          ZS_LOG_TRACE(log("no data to read"))
          return true;
        }

        while (mWireReceiveStream->getTotalReadBuffersAvailable() > 0) {
          StreamHeaderPtr header;
          SecureByteBlockPtr buffer = mWireReceiveStream->read(&header);

          ChannelHeaderPtr channelHeader = ChannelHeader::convert(header);

          if ((!buffer) ||
              (!channelHeader)) {
            ZS_LOG_WARNING(Detail, log("failed to read buffer"))
            setError(IHTTP::HTTPStatusCode_InternalServerError, "failed to read buffer");
            cancel();
            return false;
          }

          ZS_LOG_TRACE(log("received data") + ", channel number=" + string(channelHeader->mChannelID) + ", size=" + string(buffer->SizeInBytes()))

          ChannelMap::iterator found = mChannels.find(channelHeader->mChannelID);
          ChannelPtr channel;
          if (found != mChannels.end()) {
            channel = (*found).second;
          }

          if (buffer->SizeInBytes() < 1) {
            // special close request
            if (found == mChannels.end()) {
              ZS_LOG_WARNING(Detail, log("channel closed but channel is not known"))
              continue;
            }
            ZS_LOG_DEBUG(log("channel is being shutdown"))
            channel->cancel();
            continue;
          }

          if (!channel) {
            if (isFinderRelayConnection()) {
              if (0 == channelHeader->mChannelID) {
                
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
                ZS_LOG_DETAIL(log("( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ("))
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
                ZS_LOG_DETAIL(log("CHANNEL MESSAGE") + "\n" + ((CSTR)(buffer->BytePtr())) + "\n")
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))
                ZS_LOG_DETAIL(log("( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ("))
                ZS_LOG_DETAIL(log("-------------------------------------------------------------------------------------------"))

                DocumentPtr document = Document::createFromAutoDetect((CSTR)(buffer->BytePtr()));
                MessagePtr message = Message::create(document, mThisWeak.lock());

                if (!message) {
                  ZS_LOG_WARNING(Detail, log("failed to create a message object from incoming message"))
                  continue;
                }

                ZS_LOG_DETAIL(log("v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v"))
                ZS_LOG_DETAIL(log("||| MESSAGE INFO |||") + Message::toDebugString(message))
                ZS_LOG_DETAIL(log("^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^"))

                if (IMessageMonitor::handleMessageReceived(message)) {
                  ZS_LOG_DEBUG(log("handled message via message handler"))
                  continue;
                }

                ZS_LOG_WARNING(Detail, log("message not understood"))
                continue;
              }

              ZS_LOG_WARNING(Detail, log("received data on an non-mapped relay channel (thus ignoring)") + ", channel=" + string(channelHeader->mChannelID))
              continue;
            }

            ZS_LOG_DEBUG(log("notified of new incoming channel"))

            // this is s a new incoming channel
            ITransportStreamPtr receiveStream = ITransportStream::create();
            ITransportStreamPtr sendStream = ITransportStream::create();
            channel = Channel::incoming(mThisWeak.lock(), IFinderConnectionRelayChannelDelegatePtr(), receiveStream, sendStream, channelHeader->mChannelID);

            mChannels[channelHeader->mChannelID] = channel;
            mIncomingChannels[channelHeader->mChannelID] = channel;
            mSubscriptions.delegate()->onFinderConnectionIncomingRelayChannel(mThisWeak.lock());
          }

          channel->notifyDataReceived(buffer);
        }

        ZS_LOG_TRACE(log("receive complete"))
        return true;
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
        ZS_THROW_INVALID_USAGE_IF(isFinderSessionConnection())

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("already shutdown"))
          return IFinderConnectionRelayChannelPtr();
        }

        ChannelNumber channelNumber = static_cast<ChannelNumber>(zsLib::createPUID());

        ChannelPtr channel = Channel::connect(mThisWeak.lock(), delegate, localContextID, remoteContextID, relayDomain, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream, channelNumber);

        mChannels[channelNumber] = channel;
        mPendingMapRequest[channelNumber] = channel;

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();

        return channel;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection::Channel
      #pragma mark

      //-----------------------------------------------------------------------
      FinderConnection::Channel::Channel(
                                         IMessageQueuePtr queue,
                                         FinderConnectionPtr outer,
                                         IFinderConnectionRelayChannelDelegatePtr delegate,
                                         ITransportStreamPtr receiveStream,
                                         ITransportStreamPtr sendStream,
                                         ChannelNumber channelNumber
                                         ) :
        MessageQueueAssociator(queue),
        mOuter(outer),
        mCurrentState(SessionState_Pending),
        mOuterReceiveStream(receiveStream->getWriter()),
        mOuterSendStream(sendStream->getReader()),
        mChannelNumber(channelNumber)
      {
        ZS_LOG_DEBUG(log("created"))
        if (delegate) {
          mDelegate = IFinderConnectionRelayChannelDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate);
        }
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::init()
      {
        AutoRecursiveLock lock(getLock());

        mOuterReceiveStreamSubscription = mOuterReceiveStream->subscribe(mThisWeak.lock());
        mOuterSendStreamSubscription = mOuterSendStream->subscribe(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      FinderConnection::Channel::~Channel()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      FinderConnection::ChannelPtr FinderConnection::Channel::convert(IFinderConnectionRelayChannelPtr channel)
      {
        return boost::dynamic_pointer_cast<Channel>(channel);
      }

      //-----------------------------------------------------------------------
      String FinderConnection::Channel::toDebugString(IFinderConnectionRelayChannelPtr channel, bool includeCommaPrefix)
      {
        if (!channel) return String(includeCommaPrefix ? ", finder connection channel=(null)" : "finder connection channel=(null)");

        ChannelPtr pThis = Channel::convert(channel);
        return pThis->getDebugValueString(includeCommaPrefix);
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
        ZS_THROW_INVALID_ARGUMENT_IF(!outer)
        ZS_THROW_INVALID_ARGUMENT_IF(!localContextID)
        ZS_THROW_INVALID_ARGUMENT_IF(!remoteContextID)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayDomain)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessToken)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessSecretProof)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        ChannelPtr pThis(new Channel(IStackForInternal::queueStack(), outer, delegate, receiveStream, sendStream, channelNumber));
        pThis->mThisWeak = pThis;
        pThis->mConnectionInfo.mLocalContextID = String(localContextID);
        pThis->mConnectionInfo.mRemoteContextID = String(remoteContextID);
        pThis->mConnectionInfo.mRelayDomain = String(relayDomain);
        pThis->mConnectionInfo.mRelayAccessToken = String(relayAccessToken);
        pThis->mConnectionInfo.mRelayAccessSecretProof = String(relayAccessSecretProof);
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::cancel()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("cancel called"))

        if (isShutdown()) return;

        setState(SessionState_Shutdown);

        FinderConnectionPtr connection = mOuter.lock();
        if (connection) {
          connection->notifyDestroyed(mChannelNumber);
        }

        mDelegate.reset();

        mOuterReceiveStream->cancel();
        mOuterSendStream->cancel();
      }

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannel::SessionStates FinderConnection::Channel::getState(
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
        ZS_LOG_DEBUG(log("notified write ready"))
        if (writer == mOuterReceiveStream) {
          if (!mOuterStreamNotifiedReady) {
            get(mOuterStreamNotifiedReady) = true;

            FinderConnectionPtr outer = mOuter.lock();
            if (outer) {
              outer->notifyOuterWriterReady();
            }
          }
        }
        step();
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
        ZS_LOG_DEBUG(log("notified read ready"))
        step();
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
        ChannelPtr pThis(new Channel(IStackForInternal::queueStack(), outer, delegate, receiveStream, sendStream, channelNumber));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::notifyReceivedWireWriteReady()
      {
        ZS_LOG_TRACE(log("notified received wire write ready"))
        get(mWireStreamNotifiedReady) = true;
        if (mOuterSendStream) {
          mOuterSendStream->notifyReaderReadyToRead();
        }
        step();
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::notifyDataReceived(SecureByteBlockPtr buffer)
      {
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("cannot receive data as already shutdown"))
          return;
        }
        mOuterReceiveStream->write(buffer);
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::getStreams(
                                                 ITransportStreamPtr &outReceiveStream,
                                                 ITransportStreamPtr &outSendStream
                                                 )
      {
        outReceiveStream = mOuterReceiveStream->getStream();
        outSendStream = mOuterSendStream->getStream();
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
        return String("FinderConnection::Channel [" + string(mID) + "] " + message);
      }

      //-----------------------------------------------------------------------
      String FinderConnection::Channel::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;

        return
        Helper::getDebugValue("finder connection channel id", string(mID), firstTime) +
        Helper::getDebugValue("delegate", mDelegate ? String("true") : String(), firstTime) +
        Helper::getDebugValue("state", IFinderConnectionRelayChannel::toString(mCurrentState), firstTime) +
        Helper::getDebugValue("last error", 0 != mLastError ? string(mLastError) : String(), firstTime) +
        Helper::getDebugValue("last reason", mLastErrorReason, firstTime) +
        Helper::getDebugValue("channel number", string(mChannelNumber), firstTime) +
        ", outer recv stream: " + ITransportStream::toDebugString(mOuterReceiveStream->getStream(), false) +
        ", outer send stream: " + ITransportStream::toDebugString(mOuterSendStream->getStream(), false) +
        Helper::getDebugValue("outer receive stream subscription", mOuterReceiveStreamSubscription ? String("true") : String(), firstTime) +
        Helper::getDebugValue("outer send stream subscription", mOuterSendStreamSubscription ? String("true") : String(), firstTime) +
        Helper::getDebugValue("wire stream notified ready", mWireStreamNotifiedReady ? String("true") : String(), firstTime) +
        Helper::getDebugValue("outer stream notified ready", mOuterStreamNotifiedReady ? String("true") : String(), firstTime) +
        Helper::getDebugValue("local context", mConnectionInfo.mLocalContextID, firstTime) +
        Helper::getDebugValue("remote context", mConnectionInfo.mRemoteContextID, firstTime) +
        Helper::getDebugValue("relay domain", mConnectionInfo.mRelayAccessToken, firstTime) +
        Helper::getDebugValue("relay access token", mConnectionInfo.mRelayAccessToken, firstTime) +
        Helper::getDebugValue("relay access proof", mConnectionInfo.mRelayAccessSecretProof, firstTime);
      }

      //-----------------------------------------------------------------------
      void FinderConnection::Channel::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState) + getDebugValueString())

        mCurrentState = state;
        ChannelPtr pThis = mThisWeak.lock();

        if ((pThis) &&
            (mDelegate)) {
          try {
            mDelegate->onFinderConnectionRelayChannelStateChanged(pThis, mCurrentState);
          } catch (IFinderConnectionRelayChannelDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone"))
          }
        }
      }
      
      //-----------------------------------------------------------------------
      void FinderConnection::Channel::setError(WORD errorCode, const char *inReason)
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
      void FinderConnection::Channel::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step continue to shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!stepSendData()) return;

        setState(SessionState_Connected);
      }

      //-----------------------------------------------------------------------
      bool FinderConnection::Channel::stepSendData()
      {
        if (!mWireStreamNotifiedReady) {
          ZS_LOG_TRACE(log("have not received wire ready to send yet"))
          return false;
        }

        if (mOuterSendStream->getTotalReadBuffersAvailable() < 1) {
          ZS_LOG_TRACE(log("no data to send"))
          return true;
        }

        FinderConnectionPtr connection = mOuter.lock();
        if (!connection) {
          ZS_LOG_WARNING(Detail, log("connection is gone, must shutdown"))
          setError(IHTTP::HTTPStatusCode_NotFound, "connection object gone");
          cancel();
          return false;
        }

        while (mOuterSendStream->getTotalReadBuffersAvailable() > 0) {
          SecureByteBlockPtr buffer = mOuterSendStream->read();
          if (buffer->SizeInBytes() < 1) {
            ZS_LOG_WARNING(Detail, log("no data read"))
            continue;
          }

          ZS_LOG_TRACE(log("buffer to send read") + ", size=" + string(buffer->SizeInBytes()))
          connection->sendBuffer(mChannelNumber, buffer);
        }

        ZS_LOG_TRACE(log("buffer to send read complete"))
        return true;
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
      const char *IFinderConnectionRelayChannel::toString(SessionStates state)
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
