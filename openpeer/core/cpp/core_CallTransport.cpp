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


#include <openpeer/core/internal/core_CallTransport.h>
#include <openpeer/core/internal/core_Stack.h>
#include <openpeer/core/internal/core_Call.h>
#include <openpeer/core/internal/core_MediaEngineObsolete.h>
#include <openpeer/core/internal/core_MediaManager.h>
#include <openpeer/core/internal/core_Helper.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>

#define OPENPEER_CALLTRANSPORT_CLOSE_UNUSED_SOCKETS_AFTER_IN_SECONDS (90)

namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_media) } }

namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransport
      #pragma mark

      //-----------------------------------------------------------------------
      const char *ICallTransport::toString(CallTransportStates state)
      {
        switch (state) {
          case CallTransportState_Pending:      return "Pending";
          case CallTransportState_Ready:        return "Ready";
          case CallTransportState_ShuttingDown: return "Shutting down";
          case CallTransportState_Shutdown:     return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      String ICallTransport::toDebugString(ICallTransportPtr transport, bool includeCommaPrefix)
      {
        if (!transport) return includeCommaPrefix ? ", call transport=(null)" : "call transport=(null)";
        return CallTransport::convert(transport)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportForCall
      #pragma mark

      const char *ICallTransportForCall::toString(SocketTypes type)
      {
        switch (type) {
          case SocketType_Audio: return "audio";
          case SocketType_Video: return "video";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      CallTransportPtr ICallTransportForAccount::create(
                                                        ICallTransportDelegatePtr delegate,
                                                        const char *turnServer,
                                                        const char *turnServerUsername,
                                                        const char *turnServerPassword,
                                                        const char *stunServer
                                                        )
      {
        return ICallTransportFactory::singleton().create(delegate, turnServer, turnServerUsername, turnServerPassword, stunServer);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport
      #pragma mark

      //-----------------------------------------------------------------------
      CallTransport::CallTransport(
                                   IMessageQueuePtr queue,
                                   ICallTransportDelegatePtr delegate,
                                   const char *turnServer,
                                   const char *turnServerUsername,
                                   const char *turnServerPassword,
                                   const char *stunServer
                                   ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mDelegate(ICallTransportDelegateProxy::createWeak(queue, delegate)),
        mTURNServer(turnServer ? turnServer : ""),
        mTURNServerUsername(turnServerUsername ? turnServerUsername : ""),
        mTURNServerPassword(turnServerPassword ? turnServerPassword : ""),
        mSTUNServer(stunServer ? stunServer : ""),
        mCurrentState(CallTransportState_Pending),
        mTotalCalls(0),
        mFocusCallID(0),
        mFocusLocationID(0),
        mStarted(false),
        mHasAudio(false),
        mHasVideo(false),
        mBlockUntilStartStopCompleted(0)
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void CallTransport::init()
      {
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      CallTransport::~CallTransport()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      CallTransportPtr CallTransport::convert(ICallTransportPtr transport)
      {
        return boost::dynamic_pointer_cast<CallTransport>(transport);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      CallTransportPtr CallTransport::create(
                                             ICallTransportDelegatePtr delegate,
                                             const char *turnServer,
                                             const char *turnServerUsername,
                                             const char *turnServerPassword,
                                             const char *stunServer
                                             )
      {
        CallTransportPtr pThis(new CallTransport(IStackForInternal::queueMedia(), delegate, turnServer, turnServerUsername, turnServerPassword, stunServer));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void CallTransport::shutdown()
      {
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      ICallTransport::CallTransportStates CallTransport::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport => ICallTransportForCall
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &CallTransport::getLock() const
      {
        return mLock;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr CallTransport::getMediaQueue() const
      {
        return getAssociatedMessageQueue();
      }

      //-----------------------------------------------------------------------
      void CallTransport::notifyCallCreation(PUID idCall)
      {
        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("told about a new call during call transport shutdown"))
          return;
        }

        ++mTotalCalls;
        fixSockets();
      }

      //-----------------------------------------------------------------------
      void CallTransport::notifyCallDestruction(PUID idCall)
      {
        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("ignoring call destructionn during shutdown"))
          return;
        }

        ZS_THROW_BAD_STATE_IF(mTotalCalls < 1)

        --mTotalCalls;

        if (mSocketCleanupTimer) {
          mSocketCleanupTimer->cancel();
          mSocketCleanupTimer.reset();
        }

        mSocketCleanupTimer = Timer::create(mThisWeak.lock(), Seconds(OPENPEER_CALLTRANSPORT_CLOSE_UNUSED_SOCKETS_AFTER_IN_SECONDS), false);
      }

      //-----------------------------------------------------------------------
      void CallTransport::focus(
                                CallPtr call,
                                PUID locationID
                                )
      {
        CallPtr oldFocus;

        // scope: do not want to call notifyLostFocus from inside a lock (possible deadlock)
        {
          AutoRecursiveLock lock(getLock());
          ZS_THROW_BAD_STATE_IF(mTotalCalls < 1)

          if (call) {
            if (mFocusCallID != call->forCallTransport().getID()) {
              // focus has changed...

              oldFocus = mFocus.lock();

              ZS_LOG_DEBUG(log("focused call ID changed") + ", was=" + string(mFocusCallID) + ", now=" + string(call->forCallTransport().getID()) + ", block count=" + string(mBlockUntilStartStopCompleted))

              mFocus = call;
              mFocusCallID = call->forCallTransport().getID();
              mFocusLocationID = locationID;

              // must restart the media
              ++mBlockUntilStartStopCompleted;
              ICallTransportAsyncProxy::create(mThisWeak.lock())->onStart();
            } else {
              if (locationID != mFocusLocationID) {
                ZS_LOG_DEBUG(log("focused location ID changed") + ", was=" + string(mFocusLocationID) + ", now=" + string(locationID) + ", block count=" + string(mBlockUntilStartStopCompleted))

                // must restart the media
                ++mBlockUntilStartStopCompleted;
                ICallTransportAsyncProxy::create(mThisWeak.lock())->onStart();
                mFocusLocationID = locationID;
              }
            }
          } else {
            ZS_LOG_DEBUG(log("no focus at all") + ", was=" + string(mFocusLocationID) + ", now=" + string(locationID) + ", block count=" + string(mBlockUntilStartStopCompleted))
            mFocusCallID = 0;
            mFocusLocationID = 0;
            ++mBlockUntilStartStopCompleted;
            ICallTransportAsyncProxy::create(mThisWeak.lock())->onStop();
          }
        }

        if (oldFocus) {
          ZS_LOG_DEBUG(log("telling old focus to go on hold..."))
          oldFocus->forCallTransport().notifyLostFocus();
        }
      }

      //-----------------------------------------------------------------------
      void CallTransport::loseFocus(PUID callID)
      {
        AutoRecursiveLock lock(getLock());

        if (0 == mFocusCallID) {
          ZS_LOG_DEBUG(log("no call has focus (ignoring request to lose focus)") + ", lose focus ID=" + string(callID))
          return;
        }

        if (callID != mFocusCallID) {
          ZS_LOG_WARNING(Detail, log("did not have focus on current call to lose") + ", focusing on ID=" + string(mFocusCallID) + ", lose focus ID=" + string(callID))
          return;
        }

        focus(CallPtr(), 0);
      }

      //-----------------------------------------------------------------------
      IICESocketPtr CallTransport::getSocket(SocketTypes type) const
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_BAD_STATE_IF(mTotalCalls < 1)

        switch (type) {
          case SocketType_Audio:  return mAudioSocket->getRTPSocket();
          case SocketType_Video:  return mVideoSocket->getRTPSocket();
        }

        ZS_THROW_INVALID_ASSUMPTION(log("what type of socket is this?"))
        return IICESocketPtr();
      }

      //-----------------------------------------------------------------------
      void CallTransport::notifyReceivedRTPPacket(
                                                  PUID callID,
                                                  PUID locationID,
                                                  SocketTypes type,
                                                  const BYTE *buffer,
                                                  ULONG bufferLengthInBytes
                                                  )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!buffer)

        if (bufferLengthInBytes < (sizeof(BYTE)*2)) return;

        BYTE payloadType = buffer[1];
        BYTE filterType = (payloadType & 0x7F);
        bool isRTP = ((filterType < 64) || (filterType > 96));

        ZS_LOG_TRACE(log("notified of packet") + (isRTP ? ", type=RTP" : ", type=RTCP") + ", from call ID=" + string(callID) + ", from location ID=" + string(locationID) + ", type=" + ICallTransportForCall::toString(type) + ", payload type=" + string(payloadType) + ", length=" + string(bufferLengthInBytes))

        // scope - get locked variable
        {
          AutoRecursiveLock lock(getLock());

          if (0 != mBlockUntilStartStopCompleted) {
            ZS_LOG_WARNING(Debug, log("ignoring RTP/RTCP packet as media is blocked until the start/stop routine complete") + ", blocked count=" + string(mBlockUntilStartStopCompleted))
            return;
          }

          if (!mStarted) {
            ZS_LOG_TRACE(log("ignoring RTP/RTCP packet as media is not started"))
            return;
          }

          if ((callID != mFocusCallID) ||
              (locationID != mFocusLocationID)) {
            ZS_LOG_TRACE(log("ignoring RTP/RTCP packet as not from call/location ID in focus") + ", focus call ID=" + string(mFocusCallID) + ", focus location ID=" + string(mFocusLocationID))
            return;
          }

          if ((SocketType_Audio == type) &&
              (!mHasAudio)) {
            ZS_LOG_TRACE(log("ignoring RTP/RTCP packet as audio was not started for this call"))
            return;
          }

          if ((SocketType_Video == type) &&
              (!mHasVideo)) {
            ZS_LOG_TRACE(log("ignoring RTP/RTCP packet as video was not started for this call"))
            return;
          }
        }

        MediaManagerPtr mediaManager = IMediaManagerForCall::singleton();
        
        MediaSessionList::iterator sessionIter = mediaManager->forCallTransport().getMediaSessions()->begin();
        IMediaSessionPtr mediaSession = *sessionIter;
        
        if (SocketType_Audio == type) {
          MediaStreamListPtr audioStreams = mediaSession->getAudioStreams();
          for (MediaStreamList::iterator streamIter = audioStreams->begin();
               streamIter != audioStreams->end();
               streamIter++) {
            if (typeid(*streamIter) == typeid(RemoteReceiveAudioStream)) {
              RemoteReceiveAudioStreamPtr audioStream = boost::dynamic_pointer_cast<RemoteReceiveAudioStream>(*streamIter);
              ReceiveMediaTransportPtr receiveTransport = audioStream->forCallTransport().getTransport();
              if (isRTP) {
                receiveTransport->forCallTransport().receivedRTPPacket(buffer, bufferLengthInBytes);
              } else {
                receiveTransport->forCallTransport().receivedRTCPPacket(buffer, bufferLengthInBytes);
              }
            }
          }
        } else {
          MediaStreamListPtr videoStreams = mediaSession->getVideoStreams();
          for (MediaStreamList::iterator streamIter = videoStreams->begin();
               streamIter != videoStreams->end();
               streamIter++) {
            if (typeid(*streamIter) == typeid(RemoteReceiveVideoStream)) {
              RemoteReceiveVideoStreamPtr audioStream = boost::dynamic_pointer_cast<RemoteReceiveVideoStream>(*streamIter);
              ReceiveMediaTransportPtr receiveTransport = audioStream->forCallTransport().getTransport();
              if (isRTP) {
                receiveTransport->forCallTransport().receivedRTPPacket(buffer, bufferLengthInBytes);
              } else {
                receiveTransport->forCallTransport().receivedRTCPPacket(buffer, bufferLengthInBytes);
              }
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      void CallTransport::onTimer(TimerPtr timer)
      {
        AutoRecursiveLock lock(getLock());
        if (timer != mSocketCleanupTimer) {
          ZS_LOG_WARNING(Detail, log("notification from obsolete timer") + string(timer->getID()))
          return;
        }

        mSocketCleanupTimer->cancel();
        mSocketCleanupTimer.reset();

        fixSockets();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport => friend TransportSocket
      #pragma mark

      //-----------------------------------------------------------------------
      int CallTransport::sendRTPPacket(PUID socketID, const void *data, int len)
      {
        CallPtr call;
        PUID locationID = 0;

        ICallForCallTransport::SocketTypes type = ICallForCallTransport::SocketType_Audio;

        // scope - find the call
        {
          AutoRecursiveLock lock(getLock());

          if (0 != mBlockUntilStartStopCompleted) {
            ZS_LOG_WARNING(Debug, log("ignoring request to send RTP packet as media is blocked until the start/stop routine complete") + ", blocked count=" + string(mBlockUntilStartStopCompleted))
            return 0;
          }

          if ((0 == mFocusCallID) ||
              (!mStarted)) {
            ZS_LOG_WARNING(Trace, log("unable to send RTP packet media isn't start or there is no focus object") + ", started=" + (mStarted ? "true" : "false") + ", focus ID=" + string(mFocusCallID))
            return 0;
          }

          call = mFocus.lock();
          locationID = mFocusLocationID;
          if (!call) {
            ZS_LOG_WARNING(Trace, log("unable to send RTP packet as focused call object is gone"))
            return 0;
          }

          if (socketID == mAudioSocketID) {
            type = ICallForCallTransport::SocketType_Audio;
          } else {
            type = ICallForCallTransport::SocketType_Video;
          }
        }

        return (call->forCallTransport().sendRTPPacket(locationID, type, (const BYTE *)data, (ULONG)len) ? len : 0);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String CallTransport::log(const char *message) const
      {
        return String("CallTransport [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String CallTransport::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("call transport id", string(mID), firstTime) +
               Helper::getDebugValue("state", ICallTransport::toString(mCurrentState), firstTime) +
               Helper::getDebugValue("turn", mTURNServer, firstTime) +
               Helper::getDebugValue("turn username", mTURNServerUsername, firstTime) +
               Helper::getDebugValue("turn password", mTURNServerPassword, firstTime) +
               Helper::getDebugValue("stun", mSTUNServer, firstTime) +
               Helper::getDebugValue("total calls", 0 != mTotalCalls ? string(mTotalCalls) : String(), firstTime) +
               Helper::getDebugValue("socket cleanup timer", mSocketCleanupTimer ? String("true") : String(), firstTime) +
               Helper::getDebugValue("started", mStarted ? String("true") : String(), firstTime) +
               ICall::toDebugString(mFocus.lock()) +
               Helper::getDebugValue("focus call id", 0 != mFocusCallID ? string(mFocusCallID) : String(), firstTime) +
               Helper::getDebugValue("focus location id", 0 != mFocusLocationID ? string(mFocusLocationID) : String(), firstTime) +
               Helper::getDebugValue("has audio", mHasAudio ? String("true") : String(), firstTime) +
               Helper::getDebugValue("has video", mHasAudio ? String("true") : String(), firstTime) +
               Helper::getDebugValue("blocked until", 0 != mBlockUntilStartStopCompleted ? string(mBlockUntilStartStopCompleted) : String(), firstTime) +
               ", audio: " + TransportSocket::toDebugString(mAudioSocket, false) +
               ", video: " + TransportSocket::toDebugString(mVideoSocket, false) +
               Helper::getDebugValue("audio socket id", 0 != mAudioSocketID ? string(mAudioSocketID) : String(), firstTime) +
               Helper::getDebugValue("video socket id", 0 != mVideoSocketID ? string(mVideoSocketID) : String(), firstTime) +
               Helper::getDebugValue("obsolete sockets", mObsoleteSockets.size() > 0 ? string(mObsoleteSockets.size()) : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      class CallTransport_EnsureDecrementOnReturn
      {
      public:
        typedef zsLib::ULONG ULONG;
        typedef zsLib::RecursiveLock RecursiveLock;

        CallTransport_EnsureDecrementOnReturn(
                                              RecursiveLock &lock,
                                              ULONG &refCount
                                              ) :
          mLock(lock),
          mRefCount(refCount)
        {
        }

        ~CallTransport_EnsureDecrementOnReturn()
        {
          AutoRecursiveLock lock(mLock);
          --mRefCount;
        }

      private:
        RecursiveLock &mLock;
        ULONG &mRefCount;
      };

      //-----------------------------------------------------------------------
      void CallTransport::start()
      {
        CallTransport_EnsureDecrementOnReturn(mLock, mBlockUntilStartStopCompleted);

        bool hasAudio = false;
        bool hasVideo = false;
        TransportSocketPtr audioSocket;
        TransportSocketPtr videoSocket;

        // scope: media engine can't be called from within our lock or it might deadlock
        {
          AutoRecursiveLock lock(mLock);

          if (mStarted) {
            ZS_LOG_DEBUG(log("must stop existing media before starting a new focus") + ", block count=" + string(mBlockUntilStartStopCompleted))
            ++mBlockUntilStartStopCompleted;  // extra count required because we are calling manually
            stop();
          }

          if (0 == mFocusCallID) {
            ZS_LOG_DEBUG(log("no need to start audio as there is no call in focus"))
            return;
          }

          CallPtr call = mFocus.lock();
          if (!call) {
            ZS_LOG_WARNING(Detail, log("call in focus is now gone thus cannot start media engine"))
            return;
          }

          hasAudio = mHasAudio = call->forCallTransport().hasAudio();
          hasVideo = mHasVideo = call->forCallTransport().hasVideo();
          mStarted = true;

          audioSocket = mAudioSocket;
          videoSocket = mVideoSocket;

          ZS_LOG_DETAIL(log("starting media engine") + ", audio=" + (mHasAudio ? "true" : "false") + ", video=" + (mHasVideo ? "true" : "false"))
        }

        MediaManagerPtr mediaManager = IMediaManagerForCall::singleton();
        
        MediaSessionPtr mediaSession = IMediaSessionForCallTransport::create(getAssociatedMessageQueue(), IMediaSessionDelegatePtr());
        
        LocalSendAudioStreamPtr localSendAudioStream;
        RemoteReceiveAudioStreamPtr remoteReceiveAudioStream;
        
        if (hasAudio) {
          localSendAudioStream = ILocalSendAudioStreamForCallTransport::create(getAssociatedMessageQueue(), IMediaStreamDelegatePtr());
          remoteReceiveAudioStream = IRemoteReceiveAudioStreamForCallTransport::create(getAssociatedMessageQueue(), IMediaStreamDelegatePtr());
          
          ZS_LOG_DETAIL(log("registering audio media engine transports"))
          
          SendMediaTransportPtr sendTransport = localSendAudioStream->forCallTransport().getTransport();
          
          sendTransport->forCallTransport().registerExternalTransport(*(audioSocket.get()));
          
          mediaSession->forCallTransport().addStream(localSendAudioStream);
          mediaSession->forCallTransport().addStream(remoteReceiveAudioStream);
        }
        
        LocalSendVideoStreamPtr localSendVideoStream;
        RemoteReceiveVideoStreamPtr remoteReceiveVideoStream;

        if (hasVideo) {
          localSendVideoStream = ILocalSendVideoStreamForCallTransport::create(getAssociatedMessageQueue(), IMediaStreamDelegatePtr());;
          remoteReceiveVideoStream = IRemoteReceiveVideoStreamForCallTransport::create(getAssociatedMessageQueue(), IMediaStreamDelegatePtr());
          
          ZS_LOG_DETAIL(log("registering video media engine transports"))
          
          SendMediaTransportPtr transport = localSendVideoStream->forCallTransport().getTransport();
          
          transport->forCallTransport().registerExternalTransport(*(videoSocket.get()));
          
          mediaSession->forCallTransport().addStream(localSendVideoStream);
          mediaSession->forCallTransport().addStream(remoteReceiveVideoStream);
        }
        
        mediaManager->forCallTransport().addMediaSession(mediaSession);
        
        {
          AutoRecursiveLock lock(mLock);
          if (hasAudio) {
            localSendAudioStream->forCallTransport().start();
          }
          if (hasVideo) {
            localSendVideoStream->forCallTransport().start();
          }
        }
      }

      //-----------------------------------------------------------------------
      void CallTransport::stop()
      {
        CallTransport_EnsureDecrementOnReturn(mLock, mBlockUntilStartStopCompleted);

        bool hasAudio = false;
        bool hasVideo = false;

        // scope: media engine can't be called from within our lock or it might deadlock
        {
          AutoRecursiveLock lock(mLock);
          if (!mStarted) return;

          hasAudio = mHasAudio;
          hasVideo = mHasVideo;

          ZS_LOG_DETAIL(log("stopping media engine") + ", audio=" + (mHasAudio ? "true" : "false") + ", video=" + (mHasVideo ? "true" : "false"))

          mStarted = false;
          mHasAudio = false;
          mHasVideo = false;
        }
        
        MediaManagerPtr mediaManager = IMediaManagerForCall::singleton();
        
        MediaSessionList::iterator sessionIter = mediaManager->forCallTransport().getMediaSessions()->begin();
        IMediaSessionPtr mediaSession = *sessionIter;
        
        LocalSendAudioStreamPtr localSendAudioStream;
        RemoteReceiveAudioStreamPtr remoteReceiveAudioStream;
        
        MediaStreamListPtr audioStreams = mediaSession->getAudioStreams();
        for (MediaStreamList::iterator streamIter = audioStreams->begin();
             streamIter != audioStreams->end();
             streamIter++) {
          if (typeid(*streamIter) == typeid(LocalSendAudioStream)) {
            localSendAudioStream = boost::dynamic_pointer_cast<LocalSendAudioStream>(*streamIter);
          } else if (typeid(*streamIter) == typeid(RemoteReceiveAudioStream)) {
            remoteReceiveAudioStream = boost::dynamic_pointer_cast<RemoteReceiveAudioStream>(*streamIter);
          }
        }
        
        LocalSendVideoStreamPtr localSendVideoStream;
        RemoteReceiveVideoStreamPtr remoteReceiveVideoStream;
        
        MediaStreamListPtr videoStreams = mediaSession->getAudioStreams();
        for (MediaStreamList::iterator streamIter = audioStreams->begin();
             streamIter != audioStreams->end();
             streamIter++) {
          if (typeid(*streamIter) == typeid(LocalSendVideoStream)) {
            localSendVideoStream = boost::dynamic_pointer_cast<LocalSendVideoStream>(*streamIter);
          } else if (typeid(*streamIter) == typeid(RemoteReceiveVideoStream)) {
            remoteReceiveVideoStream = boost::dynamic_pointer_cast<RemoteReceiveVideoStream>(*streamIter);
          }
        }

        if (hasVideo) {
          ZS_LOG_DETAIL(log("stopping media engine video"))
          localSendVideoStream->forCallTransport().stop();
        }
        if (hasAudio) {
          ZS_LOG_DETAIL(log("stopping media engine audio"))
          localSendAudioStream->forCallTransport().stop();
        }
        
        if (hasVideo) {
          ZS_LOG_DETAIL(log("deregistering video media engine transport"))
          SendMediaTransportPtr transport = localSendVideoStream->forCallTransport().getTransport();
          transport->forCallTransport().deregisterExternalTransport();
        }
        
        if (hasAudio) {
          ZS_LOG_DETAIL(log("deregistering audio media engine transport"))
          SendMediaTransportPtr transport = localSendAudioStream->forCallTransport().getTransport();
          transport->forCallTransport().deregisterExternalTransport();
        }
      }

      //-----------------------------------------------------------------------
      void CallTransport::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("cancel called but already shutdown"))
          return;
        }

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(CallTransportState_ShuttingDown);

        mTotalCalls = 0;

        if (mAudioSocket) {
          mObsoleteSockets.push_back(mAudioSocket);
          mAudioSocket.reset();
          mAudioSocketID = 0;
        }
        if (mVideoSocket) {
          mObsoleteSockets.push_back(mVideoSocket);
          mVideoSocket.reset();
          mVideoSocketID = 0;
        }

        cleanObsoleteSockets();

        if (mGracefulShutdownReference) {
          if (!cleanObsoleteSockets()) {
            ZS_LOG_DEBUG(log("waiting for transport sockets to shutdown"))
            return;
          }
        }

        setState(CallTransportState_Shutdown);

        mGracefulShutdownReference.reset();

        mDelegate.reset();

        mObsoleteSockets.clear();
      }

      //-----------------------------------------------------------------------
      void CallTransport::step()
      {
        AutoRecursiveLock lock(getLock());
        ZS_LOG_DEBUG(log("step called"))

        cleanObsoleteSockets();

        if ((isShuttingDown()) ||
            (isShutdown())) {
          cancel();
          return;
        }

        setState(CallTransportState_Ready);
      }

      //-----------------------------------------------------------------------
      void CallTransport::setState(CallTransportStates state)
      {
        AutoRecursiveLock lock(getLock());

        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + ICallTransport::toString(mCurrentState) + ", new state=" + ICallTransport::toString(state))

        mCurrentState = state;

        CallTransportPtr pThis = mThisWeak.lock();
        if (!pThis) {
          ZS_LOG_WARNING(Detail, log("nobody holding reference to this object"))
          return;
        }

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("delegate is not set"))
          return;
        }

        try {
          mDelegate->onCallTransportStateChanged(pThis, state);
        } catch (ICallTransportDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("call transport delegate is gone"))
          mDelegate.reset();
        }
      }

      //-----------------------------------------------------------------------
      void CallTransport::fixSockets()
      {
        if (mTotalCalls > 0) {

          if (mSocketCleanupTimer) {
            mSocketCleanupTimer->cancel();
            mSocketCleanupTimer.reset();
          }

          if (!mAudioSocket) {
            ZS_LOG_DEBUG(log("creating audio sockets"))
            mAudioSocket = TransportSocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), mTURNServer, mTURNServerUsername, mTURNServerPassword, mSTUNServer);
            mAudioSocketID = mAudioSocket->getID();
          }
          if (!mVideoSocket) {
            ZS_LOG_DEBUG(log("creating video sockets"))
            mVideoSocket = TransportSocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), mTURNServer, mTURNServerUsername, mTURNServerPassword, mSTUNServer);
            mVideoSocketID = mVideoSocket->getID();
          }
          return;
        }

        if ((!mAudioSocket) && (!mVideoSocket)) {
          ZS_LOG_DEBUG(log("audio/video sockets are already closed (thus nothing to do)"))
          return;
        }

        if (mSocketCleanupTimer) {
          ZS_LOG_DEBUG(log("still have socket cleanup timer thus no need to shutdown sockets (yet)"))
          return;
        }

        if (mAudioSocket) {
          ZS_LOG_DEBUG(log("adding audio socket to obsolete socket list (for cleanup)"))
          mObsoleteSockets.push_back(mAudioSocket);
          mAudioSocketID = 0;
          mAudioSocket.reset();
        }

        if (mVideoSocket) {
          ZS_LOG_DEBUG(log("adding video socket to obsolete socket list (for cleanup)"))
          mObsoleteSockets.push_back(mVideoSocket);
          mVideoSocketID = 0;
          mVideoSocket.reset();
        }

        cleanObsoleteSockets();
      }

      //-----------------------------------------------------------------------
      bool CallTransport::cleanObsoleteSockets()
      {
        for (TransportSocketList::iterator obsoleteIter = mObsoleteSockets.begin(); obsoleteIter != mObsoleteSockets.end();)
        {
          TransportSocketList::iterator current = obsoleteIter;
          ++obsoleteIter;

          TransportSocketPtr &socket = (*current);

          socket->shutdown();

          if (socket->isShutdown()) {
            ZS_LOG_DEBUG(log("transport socket is now shutdown") + ", transport socket ID=" + string(socket->getID()))
            mObsoleteSockets.erase(current);
            continue;
          }

          ZS_LOG_DEBUG(log("transport socket is still shutting down") + ", transport socket ID=" + string(socket->getID()))
        }

        return mObsoleteSockets.size() < 1;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport::TransportSocket
      #pragma mark

      CallTransport::TransportSocket::TransportSocket(
                                                      IMessageQueuePtr queue,
                                                      CallTransportPtr outer
                                                      ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(outer)
      {
      }

      //-----------------------------------------------------------------------
      void CallTransport::TransportSocket::init(
                                                const char *turnServer,
                                                const char *turnServerUsername,
                                                const char *turnServerPassword,
                                                const char *stunServer
                                                )
      {
        mRTPSocket = IICESocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), turnServer, turnServerUsername, turnServerPassword, stunServer, 0, true);
      }

      //-----------------------------------------------------------------------
      CallTransport::TransportSocket::~TransportSocket()
      {
        mThisWeak.reset();
        if (mRTPSocket) {
          mRTPSocket->shutdown();
          mRTPSocket.reset();
        }
      }

      //-----------------------------------------------------------------------
      String CallTransport::TransportSocket::toDebugString(TransportSocketPtr socket, bool includeCommaPrefix)
      {
        if (!socket) return includeCommaPrefix ? ", transport socket=(null)" : "transport socket=(null)";
        return socket->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport::TransportSocket => friend CallTransport
      #pragma mark

      CallTransport::TransportSocketPtr CallTransport::TransportSocket::create(
                                                                               IMessageQueuePtr queue,
                                                                               CallTransportPtr outer,
                                                                               const char *turnServer,
                                                                               const char *turnServerUsername,
                                                                               const char *turnServerPassword,
                                                                               const char *stunServer
                                                                               )
      {
        TransportSocketPtr pThis(new TransportSocket(queue, outer));
        pThis->mThisWeak = pThis;
        pThis->init(turnServer, turnServerUsername, turnServerPassword, stunServer);
        return pThis;
      }

      //-----------------------------------------------------------------------
      void CallTransport::TransportSocket::shutdown()
      {
        if (mRTPSocket) {
          mRTPSocket->shutdown();
          if (IICESocket::ICESocketState_Shutdown == mRTPSocket->getState()) {
            mRTPSocket.reset();
          }
        }
      }

      //-----------------------------------------------------------------------
      bool CallTransport::TransportSocket::isReady() const
      {
        if (!mRTPSocket) return false;

        if (IICESocket::ICESocketState_Ready != mRTPSocket->getState()) return false;
        return true;
      }

      //-----------------------------------------------------------------------
      bool CallTransport::TransportSocket::isShutdown() const
      {
        if (mRTPSocket) {
          if (IICESocket::ICESocketState_Shutdown != mRTPSocket->getState()) return false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport::TransportSocket => IICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void CallTransport::TransportSocket::onICESocketStateChanged(
                                                                   IICESocketPtr socket,
                                                                   ICESocketStates state
                                                                   )
      {
        CallTransportPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("ICE state change ignored as call transport object is gone"))
          return;
        }

        ZS_LOG_DEBUG(log("on ice socket state changed"))
        IWakeDelegateProxy::create(outer)->onWake();
      }

      //-----------------------------------------------------------------------
      void CallTransport::TransportSocket::onICESocketCandidatesChanged(IICESocketPtr socket)
      {
        CallTransportPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("ICE candidates change ignored as call transport object is gone"))
          return;
        }

        ZS_LOG_DEBUG(log("on ice socket candidates changed"))
        IWakeDelegateProxy::create(outer)->onWake();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport::TransportSocket->webrtc::Transport
      #pragma mark

      //-----------------------------------------------------------------------
      int CallTransport::TransportSocket::SendPacket(int channel, const void *data, int len)
      {
        if (len < (sizeof(BYTE)*2)) return 0;

        BYTE payloadType = ((const BYTE *)data)[1];
        ZS_LOG_TRACE(log("request to send RTP packet") + ", payload type=" + string(payloadType) + ", length=" + string(len))

        CallTransportPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_TRACE(log("cannot send RTP packet because call transport object is gone"))
          return 0;
        }

        return outer->sendRTPPacket(mID, data, len);
      }

      //-----------------------------------------------------------------------
      int CallTransport::TransportSocket::SendRTCPPacket(int channel, const void *data, int len)
      {
        if (len < (sizeof(BYTE)*2)) return 0;

        BYTE payloadType = ((const BYTE *)data)[1];
        ZS_LOG_TRACE(log("request to send RTCP packet") + ", payload type=" + string(payloadType) + ", length=" + string(len))

        CallTransportPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Trace, log("cannot send RTCP packet because call transport object is gone"))
          return 0;
        }

        return outer->sendRTPPacket(mID, data, len);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark CallTransport::TransportSocket => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String CallTransport::TransportSocket::log(const char *message) const
      {
        return String("CallTransport::TransportSocket [") + string(mID) + "] " + message;
      }

      //-----------------------------------------------------------------------
      String CallTransport::TransportSocket::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("transport socket id", string(mID), firstTime) +
               Helper::getDebugValue("rtp socket id", mRTPSocket ? string(mRTPSocket->getID()) : String(), firstTime);
      }
    }
  }
}
