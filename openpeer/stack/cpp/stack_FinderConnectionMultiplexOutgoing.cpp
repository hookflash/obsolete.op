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

#include <openpeer/stack/internal/stack_FinderConnectionMultiplexOutgoing.h>
#include <openpeer/stack/internal/stack_Helper.h>
#include <openpeer/stack/internal/stack_Stack.h>
#include <openpeer/stack/internal/stack_FinderRelayChannel.h>

#include <openpeer/stack/IMessageMonitor.h>

#include <openpeer/services/IHelper.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>

#include <zsLib/Stringize.h>

#define OPENPEER_STACK_FINDER_RELAY_MULTIPLEX_OUTGOING_RECEIVE_INACTIVITY_TIMEOUT_IN_SECONDS (60*20)

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
      #pragma mark FinderConnectionMultiplexOutgoingManager
      #pragma mark

      class FinderConnectionMultiplexOutgoingManager
      {
      public:
        friend class FinderConnectionMultiplexOutgoing;

        typedef String RemoteIPString;
        typedef std::map<RemoteIPString, FinderConnectionMultiplexOutgoingPtr> FinderConnectionMultiplexOutgoingMap;

      protected:
        //---------------------------------------------------------------------
        FinderConnectionMultiplexOutgoingManager()
        {
        }

        //---------------------------------------------------------------------
        void init()
        {
        }

        //---------------------------------------------------------------------
        static FinderConnectionMultiplexOutgoingManagerPtr create()
        {
          FinderConnectionMultiplexOutgoingManagerPtr pThis(new FinderConnectionMultiplexOutgoingManager);
          return pThis;
        }

      public:
        //---------------------------------------------------------------------
        static FinderConnectionMultiplexOutgoingManagerPtr singleton()
        {
          AutoRecursiveLock lock(IHelper::getGlobalLock());
          static FinderConnectionMultiplexOutgoingManagerPtr singleton = create();
          return singleton;
        }

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoingManager => friend FinderConnectionMultiplexOutgoing
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        FinderConnectionMultiplexOutgoingPtr find(const IPAddress &remoteIP)
        {
          FinderConnectionMultiplexOutgoingMap::iterator found = mRelays.find(remoteIP.string());
          if (found == mRelays.end()) return FinderConnectionMultiplexOutgoingPtr();

          return (*found).second;
        }

        //---------------------------------------------------------------------
        void add(
                 const IPAddress &remoteIP,
                 FinderConnectionMultiplexOutgoingPtr relay
                 )
        {
          mRelays[remoteIP.string()] = relay;
        }

        //---------------------------------------------------------------------
        void remove(const IPAddress &remoteIP)
        {
          FinderConnectionMultiplexOutgoingMap::iterator found = mRelays.find(remoteIP.string());
          if (found == mRelays.end()) return;

          mRelays.erase(found);
        }

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoingManager => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        virtual RecursiveLock &getLock() const {return mLock;}

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoingManager => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        FinderConnectionMultiplexOutgoingManagerWeakPtr mThisWeak;

        FinderConnectionMultiplexOutgoingMap mRelays;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing
      #pragma mark

      //-----------------------------------------------------------------------
      FinderConnectionMultiplexOutgoing::FinderConnectionMultiplexOutgoing(
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
      void FinderConnectionMultiplexOutgoing::init()
      {
        AutoRecursiveLock lock(getLock());

        mWireReceiveStream = ITransportStream::create()->getReader();
        mWireSendStream = ITransportStream::create()->getWriter();

        mTCPMessaging->connect(mThisWeak.lock(), mWireReceiveStream->getStream(), mWireSendStream->getStream(), true, mRemoteIP);

        mInactivityTimer = Timer::create(mThisWeak.lock(), Seconds(1));

        step();
      }

      //-----------------------------------------------------------------------
      FinderConnectionMultiplexOutgoing::~FinderConnectionMultiplexOutgoing()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      FinderConnectionMultiplexOutgoingPtr FinderConnectionMultiplexOutgoing::convert(IFinderConnectionPtr connection)
      {
        return boost::dynamic_pointer_cast<FinderConnectionMultiplexOutgoing>(connection);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing => friend FinderConnectionMultiplexOutgoingManager
      #pragma mark

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannelPtr FinderConnectionMultiplexOutgoing::connect(
                                                                                  IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                                  const IPAddress &remoteFinderIP,
                                                                                  const char *localContextID,
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
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessToken)
        ZS_THROW_INVALID_ARGUMENT_IF(!relayAccessSecretProof)
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        FinderConnectionMultiplexOutgoingManagerPtr manager = FinderConnectionMultiplexOutgoingManager::singleton();

        AutoRecursiveLock lock(manager->getLock());

        FinderConnectionMultiplexOutgoingPtr existing = manager->find(remoteFinderIP);

        if (existing) {
          ZS_LOG_DEBUG(existing->log("reusing existing connection"))
          return existing->connect(delegate, localContextID, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream);
        }

        FinderConnectionMultiplexOutgoingPtr pThis = FinderConnectionMultiplexOutgoing::create(remoteFinderIP);
        pThis->mThisWeak = pThis;
        pThis->mOuter = manager;
        pThis->init();

        manager->add(remoteFinderIP, pThis);

        return pThis->connect(delegate, localContextID, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing => IFinderConnection
      #pragma mark

      //-----------------------------------------------------------------------
      String FinderConnectionMultiplexOutgoing::toDebugString(IFinderConnectionPtr connection, bool includeCommaPrefix)
      {
        if (!connection) return String(includeCommaPrefix ? ", finder connection multiplex outgoing=(null)" : "finder connection multiplex outgoing=(null)");

        FinderConnectionMultiplexOutgoingPtr pThis = FinderConnectionMultiplexOutgoing::convert(connection);
        return pThis->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      IFinderConnectionPtr FinderConnectionMultiplexOutgoing::connect(
                                                                      IFinderConnectionDelegatePtr delegate,
                                                                      const IPAddress &remoteFinderIP,
                                                                      ITransportStreamPtr receiveStream,
                                                                      ITransportStreamPtr sendStream
                                                                      )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!receiveStream)
        ZS_THROW_INVALID_ARGUMENT_IF(!sendStream)

        FinderConnectionMultiplexOutgoingPtr pThis = FinderConnectionMultiplexOutgoing::create(remoteFinderIP);
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
      IFinderConnectionSubscriptionPtr FinderConnectionMultiplexOutgoing::subscribe(IFinderConnectionDelegatePtr originalDelegate)
      {
        ZS_LOG_TRACE(log("subscribe called"))

        AutoRecursiveLock lock(getLock());

        if (!originalDelegate) return mDefaultSubscription;

        IFinderConnectionSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate, IStackForInternal::queueDelegate());

        IFinderConnectionDelegatePtr delegate = mSubscriptions.delegate(subscription);

        if (delegate) {
          FinderConnectionMultiplexOutgoingPtr pThis(mThisWeak.lock());

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
      void FinderConnectionMultiplexOutgoing::cancel()
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

        FinderConnectionMultiplexOutgoingManagerPtr outer = mOuter.lock();
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
      FinderConnectionMultiplexOutgoing::SessionStates FinderConnectionMultiplexOutgoing::getState(
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
      IFinderRelayChannelPtr FinderConnectionMultiplexOutgoing::accept(
                                                                       IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
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

        FinderRelayChannelPtr relay = IFinderRelayChannelForFinderConnectionMultiplexOutgoing::createIncoming(delegate, account, receiveStream, sendStream, wireReceiveStream, wireSendStream);

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
      #pragma mark FinderConnectionMultiplexOutgoing => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::onTimer(TimerPtr timer)
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
      void FinderConnectionMultiplexOutgoing::onWake()
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
      #pragma mark FinderConnectionMultiplexOutgoing => ITCPMessagingDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::onTCPMessagingStateChanged(
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
      #pragma mark FinderConnectionMultiplexOutgoing => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
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
      #pragma mark FinderConnectionMultiplexOutgoing => ITransportStreamReaderDelegate
      #pragma mark

      void FinderConnectionMultiplexOutgoing::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
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
      #pragma mark FinderConnectionMultiplexOutgoing => IFinderConnectionRelayChannelDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::onFinderRelayChannelTCPOutgoingStateChanged(
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
      #pragma mark FinderConnectionMultiplexOutgoing => IMessageMonitorResultDelegate<ChannelMapResult>
      #pragma mark

      //-----------------------------------------------------------------------
      bool FinderConnectionMultiplexOutgoing::handleMessageMonitorResultReceived(
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

        // map request completed successfully...

        return true;
      }

      //-----------------------------------------------------------------------
      bool FinderConnectionMultiplexOutgoing::handleMessageMonitorErrorResultReceived(
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

        mMapRequestChannelMonitor.reset();

        // map request failed... clear out this channel
        notifyDestroyed(mMapRequestChannelNumber);

        get(mMapRequestChannelNumber) = 0;
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing => friend ChannelOutgoing
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::sendBuffer(
                                                         ChannelNumber channelNumber,
                                                         SecureByteBlockPtr buffer
                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!buffer)

        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("send buffer called") + ", channel number=" + string(channelNumber) + ", buffer size=" + string(buffer->SizeInBytes()))

        ChannelHeaderPtr header(new ChannelHeader);
        header->mChannelID = channelNumber;

        mWireSendStream->write(buffer, header);
      }

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::notifyDestroyed(ChannelNumber channelNumber)
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
      #pragma mark FinderConnectionMultiplexOutgoing => (internal)
      #pragma mark


      //-----------------------------------------------------------------------
      RecursiveLock &FinderConnectionMultiplexOutgoing::getLock() const
      {
        FinderConnectionMultiplexOutgoingManagerPtr outer = mOuter.lock();
        if (!outer) return mLocalLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String FinderConnectionMultiplexOutgoing::log(const char *message) const
      {
        return String("FinderConnectionMultiplexOutgoing [" + string(mID) + "] " + message);
      }

      //-----------------------------------------------------------------------
      String FinderConnectionMultiplexOutgoing::getDebugValueString(bool includeCommaPrefix) const
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
        Helper::getDebugValue("remove channels", mRemoveChannels.size() > 0 ? string(mRemoveChannels.size()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::setState(SessionStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_DEBUG(log("state changed") + ", state=" + toString(state) + ", old state=" + toString(mCurrentState))
        mCurrentState = state;
        FinderConnectionMultiplexOutgoingPtr pThis = mThisWeak.lock();

        if (pThis) {
          mSubscriptions.delegate()->onFinderConnectionStateChanged(pThis, mCurrentState);
        }
      }

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::setError(WORD errorCode, const char *inReason)
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
      void FinderConnectionMultiplexOutgoing::step()
      {
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("step continue to shutdown"))
          cancel();
          return;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString())

        if (!stepCleanRemoval()) return;

        setState(SessionState_Connected);
      }

      //-----------------------------------------------------------------------
      bool FinderConnectionMultiplexOutgoing::stepCleanRemoval()
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
      FinderConnectionMultiplexOutgoingPtr FinderConnectionMultiplexOutgoing::create(IPAddress remoteFinderIP)
      {
        //TODO
        return FinderConnectionMultiplexOutgoingPtr();
      }

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannelPtr FinderConnectionMultiplexOutgoing::connect(
                                                                                  IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                                  const char *localContextID,
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
      #pragma mark FinderConnectionMultiplexOutgoing::Channel
      #pragma mark

      FinderConnectionMultiplexOutgoing::Channel::Channel(
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
      void FinderConnectionMultiplexOutgoing::Channel::init()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      FinderConnectionMultiplexOutgoing::Channel::~Channel()
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      FinderConnectionMultiplexOutgoing::ChannelPtr FinderConnectionMultiplexOutgoing::Channel::convert(IFinderConnectionRelayChannelPtr channel)
      {
        //TODO
        return ChannelPtr();
      }

      //-----------------------------------------------------------------------
      String FinderConnectionMultiplexOutgoing::Channel::toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix)
      {
        //TOOD
        return String();
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing::Channel => IFinderConnectionRelayChannel
      #pragma mark

      //-----------------------------------------------------------------------
      FinderConnectionMultiplexOutgoing::ChannelPtr FinderConnectionMultiplexOutgoing::Channel::connect(
                                                                                                        FinderConnectionMultiplexOutgoingPtr outer,
                                                                                                        IFinderConnectionRelayChannelDelegatePtr delegate,
                                                                                                        const char *localContextID,
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
      void FinderConnectionMultiplexOutgoing::Channel::cancel()
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      IFinderConnectionRelayChannel::SessionStates FinderConnectionMultiplexOutgoing::Channel::getState(
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
      #pragma mark FinderConnectionMultiplexOutgoing::Channel => ITransportStreamWriterDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::Channel::onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing::Channel => ITransportStreamReaderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::Channel::onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
      {
        AutoRecursiveLock lock(getLock());
        //TODO
      }

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing::Channel => friend FinderConnectionMultiplexOutgoing
      #pragma mark

      FinderConnectionMultiplexOutgoing::ChannelPtr FinderConnectionMultiplexOutgoing::Channel::incoming(
                                                                                                         FinderConnectionMultiplexOutgoingPtr outer,
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
      void FinderConnectionMultiplexOutgoing::Channel::notifyReceivedWireWriteReady()
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::Channel::notifyDataReceived(SecureByteBlockPtr buffer)
      {
        //TODO
      }

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::Channel::getStreams(
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
      #pragma mark FinderConnectionMultiplexOutgoing::Channel => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &FinderConnectionMultiplexOutgoing::Channel::getLock() const
      {
        FinderConnectionMultiplexOutgoingPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }
      
      //-----------------------------------------------------------------------
      String FinderConnectionMultiplexOutgoing::Channel::log(const char *message) const
      {
        //TODO
        return String();
      }

      //-----------------------------------------------------------------------
      String FinderConnectionMultiplexOutgoing::Channel::getDebugValueString(bool includeCommaPrefix) const
      {
        //TODO
        return String();
      }

      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::Channel::setState(SessionStates state)
      {
        //TODO
      }
      
      //-----------------------------------------------------------------------
      void FinderConnectionMultiplexOutgoing::Channel::setError(WORD errorCode, const char *inReason)
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
                                                                              const char *relayAccessToken,
                                                                              const char *relayAccessSecretProof,
                                                                              ITransportStreamPtr receiveStream,
                                                                              ITransportStreamPtr sendStream
                                                                              )
      {
        return internal::IFinderConnectionRelayChannelFactory::singleton().connect(delegate, remoteFinderIP, localContextID, relayAccessToken, relayAccessSecretProof, receiveStream, sendStream);
      }
    }
  }
}
