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


#include <hookflash/core/internal/core_CallTransport.h>
#include <hookflash/core/internal/core_Stack.h>
#include <hookflash/core/internal/core_Call.h>
#include <hookflash/core/internal/core_MediaEngine.h>
#include <hookflash/core/internal/core_Helper.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>

#define HOOKFLASH_CALLTRANSPORT_CLOSE_UNUSED_SOCKETS_AFTER_IN_SECONDS (90)

namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_media) } }

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Stringize;

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
        ICallTransportAsyncProxy::create(mThisWeak.lock())->onStep();
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

        mSocketCleanupTimer = Timer::create(mThisWeak.lock(), Seconds(HOOKFLASH_CALLTRANSPORT_CLOSE_UNUSED_SOCKETS_AFTER_IN_SECONDS), false);
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

              ZS_LOG_DEBUG(log("focused call ID changed") + ", was=" + Stringize<PUID>(mFocusCallID).string() + ", now=" + Stringize<PUID>(call->forCallTransport().getID()).string() + ", block count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())

              mFocus = call;
              mFocusCallID = call->forCallTransport().getID();
              mFocusLocationID = locationID;

              // must restart the media
              ++mBlockUntilStartStopCompleted;
              ICallTransportAsyncProxy::create(mThisWeak.lock())->onStart();
            } else {
              if (locationID != mFocusLocationID) {
                ZS_LOG_DEBUG(log("focused location ID changed") + ", was=" + Stringize<PUID>(mFocusLocationID).string() + ", now=" + Stringize<PUID>(locationID).string() + ", block count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())

                // must restart the media
                ++mBlockUntilStartStopCompleted;
                ICallTransportAsyncProxy::create(mThisWeak.lock())->onStart();
                mFocusLocationID = locationID;
              }
            }
          } else {
            ZS_LOG_DEBUG(log("no focus at all") + ", was=" + Stringize<PUID>(mFocusLocationID).string() + ", now=" + Stringize<PUID>(locationID).string() + ", block count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())
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
          ZS_LOG_DEBUG(log("no call has focus (ignoring request to lose focus)") + ", lose focus ID=" + Stringize<PUID>(callID).string())
          return;
        }

        if (callID != mFocusCallID) {
          ZS_LOG_WARNING(Detail, log("did not have focus on current call to lose") + ", focusing on ID=" + Stringize<PUID>(mFocusCallID).string() + ", lose focus ID=" + Stringize<PUID>(callID).string())
          return;
        }

        focus(CallPtr(), 0);
      }

      //-----------------------------------------------------------------------
      IICESocketPtr CallTransport::getSocket(
                                             SocketTypes type,
                                             bool forRTP
                                             ) const
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_BAD_STATE_IF(mTotalCalls < 1)

        switch (type) {
          case SocketType_Audio:  return (forRTP ? mAudioSocket->getRTPSocket() : mAudioSocket->getRTCPSocket());
          case SocketType_Video:  return (forRTP ? mVideoSocket->getRTPSocket() : mVideoSocket->getRTCPSocket());
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

        ZS_LOG_TRACE(log("notified of RTP packet") + ", from call ID=" + Stringize<PUID>(callID).string() + ", from location ID=" + Stringize<PUID>(locationID).string() + ", type=" + ICallTransportForCall::toString(type) + ", payload type=" + Stringize<BYTE>(payloadType).string() + ", length=" + Stringize<ULONG>(bufferLengthInBytes).string())

        // scope - get locked variable
        {
          AutoRecursiveLock lock(getLock());

          if (0 != mBlockUntilStartStopCompleted) {
            ZS_LOG_WARNING(Debug, log("ignoring RTP packet as media is blocked until the start/stop routine complete") + ", blocked count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())
            return;
          }

          if (!mStarted) {
            ZS_LOG_TRACE(log("ignoring RTP packet as media is not started"))
            return;
          }

          if ((callID != mFocusCallID) ||
              (locationID != mFocusLocationID)) {
            ZS_LOG_TRACE(log("ignoring RTP packet as not from call/location ID in focus") + ", focus call ID=" + Stringize<PUID>(mFocusCallID).string() + ", focus location ID=" + Stringize<PUID>(mFocusLocationID).string())
            return;
          }

          if ((SocketType_Audio == type) &&
              (!mHasAudio)) {
            ZS_LOG_TRACE(log("ignoring RTP packet as audio was not started for this call"))
            return;
          }

          if ((SocketType_Video == type) &&
              (!mHasVideo)) {
            ZS_LOG_TRACE(log("ignoring RTP packet as video was not started for this call"))
            return;
          }
        }

        MediaEnginePtr engine = IMediaEngineForCallTransport::singleton();

        if (SocketType_Audio == type) {
          engine->forCallTransport().receivedVoiceRTPPacket(buffer, bufferLengthInBytes);
        } else {
          engine->forCallTransport().receivedVideoRTPPacket(buffer, bufferLengthInBytes);
        }
      }

      //-----------------------------------------------------------------------
      void CallTransport::notifyReceivedRTCPPacket(
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

        ZS_LOG_TRACE(log("notified of RTCP packet") + ", from call ID=" + Stringize<PUID>(callID).string() + ", from location ID=" + Stringize<PUID>(locationID).string() + ", type=" + ICallTransportForCall::toString(type) + ", payload type=" + Stringize<BYTE>(payloadType).string() + ", length=" + Stringize<ULONG>(bufferLengthInBytes).string())

        // scope - get locked variable
        {
          AutoRecursiveLock lock(getLock());

          if (0 != mBlockUntilStartStopCompleted) {
            ZS_LOG_WARNING(Debug, log("ignoring RTCP packet as media is blocked until the start/stop routine complete") + ", blocked count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())
            return;
          }

          if (!mStarted) {
            ZS_LOG_WARNING(Trace, log("ignoring RTCP packet as media is not started"))
            return;
          }

          if ((callID != mFocusCallID) ||
              (locationID != mFocusLocationID)) {
            ZS_LOG_TRACE(log("ignoring RTCP packet as not from call/location ID in focus") + ", focus call ID=" + Stringize<PUID>(mFocusCallID).string() + ", focus location ID=" + Stringize<PUID>(mFocusLocationID).string())
            return;
          }

          if ((SocketType_Audio == type) &&
              (!mHasAudio)) {
            ZS_LOG_WARNING(Trace, log("ignoring RTCP packet as audio was not started for this call"))
            return;
          }
          if ((SocketType_Video == type) &&
              (!mHasVideo)) {
            ZS_LOG_WARNING(Trace, log("ignoring RTCP packet as video was not started for this call"))
            return;
          }
        }

        MediaEnginePtr engine = IMediaEngineForCallTransport::singleton();

        if (SocketType_Audio == type) {
          engine->forCallTransport().receivedVoiceRTCPPacket(buffer, bufferLengthInBytes);
        } else {
          engine->forCallTransport().receivedVideoRTCPPacket(buffer, bufferLengthInBytes);
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
          ZS_LOG_WARNING(Detail, log("notification from obsolete timer") + Stringize<PUID>(timer->getID()).string())
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
            ZS_LOG_WARNING(Debug, log("ignoring request to send RTP packet as media is blocked until the start/stop routine complete") + ", blocked count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())
            return 0;
          }

          if ((0 == mFocusCallID) ||
              (!mStarted)) {
            ZS_LOG_WARNING(Trace, log("unable to send RTP packet media isn't start or there is no focus object") + ", started=" + (mStarted ? "true" : "false") + ", focus ID=" + Stringize<PUID>(mFocusCallID).string())
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
      int CallTransport::sendRTCPPacket(PUID socketID, const void *data, int len)
      {
        CallPtr call;
        PUID locationID = 0;

        ICallForCallTransport::SocketTypes type = ICallForCallTransport::SocketType_Audio;

        // scope - find the call
        {
          AutoRecursiveLock lock(getLock());
          if (0 != mBlockUntilStartStopCompleted) {
            ZS_LOG_WARNING(Debug, log("ignoring request to send RTCP packet as media is blocked until the start/stop routine complete") + ", blocked count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())
            return 0;
          }

          if ((0 == mFocusCallID) ||
              (!mStarted)) {
            ZS_LOG_WARNING(Trace, log("unable to send RTCP packet media isn't start or there is no focus object") + ", started=" + (mStarted ? "true" : "false") + ", focus ID=" + Stringize<PUID>(mFocusCallID).string())
            return 0;
          }

          call = mFocus.lock();
          locationID = mFocusLocationID;
          if (!call) {
            ZS_LOG_WARNING(Trace, log("unable to send RTCP packet as focused call object is gone"))
            return 0;
          }

          if (socketID == mAudioSocketID) {
            type = ICallForCallTransport::SocketType_Audio;
          } else {
            type = ICallForCallTransport::SocketType_Video;
          }
        }

        return (call->forCallTransport().sendRTCPPacket(locationID, type, (const BYTE *)data, (ULONG)len) ? len : 0);
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
        return String("CallTransport [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String CallTransport::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("call transport id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("state", ICallTransport::toString(mCurrentState), firstTime) +
               Helper::getDebugValue("turn", mTURNServer, firstTime) +
               Helper::getDebugValue("turn username", mTURNServerUsername, firstTime) +
               Helper::getDebugValue("turn password", mTURNServerPassword, firstTime) +
               Helper::getDebugValue("stun", mSTUNServer, firstTime) +
               Helper::getDebugValue("total calls", 0 != mTotalCalls ? Stringize<typeof(mTotalCalls)>(mTotalCalls).string() : String(), firstTime) +
               Helper::getDebugValue("socket cleanup timer", mSocketCleanupTimer ? String("true") : String(), firstTime) +
               Helper::getDebugValue("started", mStarted ? String("true") : String(), firstTime) +
               ICall::toDebugString(mFocus.lock()) +
               Helper::getDebugValue("focus call id", 0 != mFocusCallID ? Stringize<typeof(mFocusCallID)>(mFocusCallID).string() : String(), firstTime) +
               Helper::getDebugValue("focus location id", 0 != mFocusLocationID ? Stringize<typeof(mFocusLocationID)>(mFocusLocationID).string() : String(), firstTime) +
               Helper::getDebugValue("has audio", mHasAudio ? String("true") : String(), firstTime) +
               Helper::getDebugValue("has video", mHasAudio ? String("true") : String(), firstTime) +
               Helper::getDebugValue("blocked until", 0 != mBlockUntilStartStopCompleted ? Stringize<typeof(mBlockUntilStartStopCompleted)>(mBlockUntilStartStopCompleted).string() : String(), firstTime) +
               ", audio: " + TransportSocket::toDebugString(mAudioSocket, false) +
               ", video: " + TransportSocket::toDebugString(mVideoSocket, false) +
               Helper::getDebugValue("audio socket id", 0 != mAudioSocketID ? Stringize<typeof(mAudioSocketID)>(mAudioSocketID).string() : String(), firstTime) +
               Helper::getDebugValue("video socket id", 0 != mVideoSocketID ? Stringize<typeof(mAudioSocketID)>(mVideoSocketID).string() : String(), firstTime) +
               Helper::getDebugValue("obsolete sockets", mObsoleteSockets.size() > 0 ? Stringize<size_t>(mObsoleteSockets.size()).string() : String(), firstTime);
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
            ZS_LOG_DEBUG(log("must stop existing media before starting a new focus") + ", block count=" + Stringize<ULONG>(mBlockUntilStartStopCompleted).string())
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

        MediaEnginePtr engine = IMediaEngineForCallTransport::singleton();

        if (hasAudio) {
          ZS_LOG_DETAIL(log("registering audio media engine transports"))

          engine->forCallTransport().registerVoiceExternalTransport(*(audioSocket.get()));
        }
        if (hasVideo) {
          ZS_LOG_DETAIL(log("registering video media engine transports"))
          engine->forCallTransport().registerVideoExternalTransport(*(videoSocket.get()));
        }

        {
          AutoRecursiveLock lock(mLock);
          if (hasAudio) {
            engine->forCallTransport().startVoice();
          }
          if (hasVideo) {
            engine->forCallTransport().startVideo();
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

        MediaEnginePtr engine = boost::dynamic_pointer_cast<MediaEngine>(IMediaEngine::singleton());

        if (hasVideo) {
          ZS_LOG_DETAIL(log("stopping media engine video"))
          engine->forCallTransport().stopVideo();
        }
        if (hasAudio) {
          ZS_LOG_DETAIL(log("stopping media engine audio"))
          engine->forCallTransport().stopVoice();
        }

        if (hasVideo) {
          ZS_LOG_DETAIL(log("deregistering video media engine transport"))
          engine->forCallTransport().deregisterVideoExternalTransport();
        }

        if (hasAudio) {
          ZS_LOG_DETAIL(log("deregistering audio media engine transport"))
          engine->forCallTransport().deregisterVoiceExternalTransport();
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
            ZS_LOG_DEBUG(log("transport socket is now shutdown") + ", transport socket ID=" + Stringize<PUID>(socket->getID()).string())
            mObsoleteSockets.erase(current);
            continue;
          }

          ZS_LOG_DEBUG(log("transport socket is still shutting down") + ", transport socket ID=" + Stringize<PUID>(socket->getID()).string())
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
        mRTPSocket = IICESocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), turnServer, turnServerUsername, turnServerPassword, stunServer);
        mRTCPSocket = IICESocket::create(getAssociatedMessageQueue(), mThisWeak.lock(), turnServer, turnServerUsername, turnServerPassword, stunServer);

        mRTCPSocket->setFoundation(mRTPSocket);
      }

      //-----------------------------------------------------------------------
      CallTransport::TransportSocket::~TransportSocket()
      {
        mThisWeak.reset();
        if (mRTPSocket) {
          mRTPSocket->shutdown();
          mRTPSocket.reset();
        }
        if (mRTCPSocket) {
          mRTCPSocket->shutdown();
          mRTCPSocket.reset();
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
        if (mRTCPSocket) {
          mRTCPSocket->shutdown();
          if (IICESocket::ICESocketState_Shutdown == mRTCPSocket->getState()) {
            mRTCPSocket.reset();
          }
        }
      }

      //-----------------------------------------------------------------------
      bool CallTransport::TransportSocket::isReady() const
      {
        if (!mRTPSocket) return false;
        if (!mRTCPSocket) return false;

        if (IICESocket::ICESocketState_Ready != mRTPSocket->getState()) return false;
        if (IICESocket::ICESocketState_Ready != mRTCPSocket->getState()) return false;
        return true;
      }

      //-----------------------------------------------------------------------
      bool CallTransport::TransportSocket::isShutdown() const
      {
        if (mRTPSocket) {
          if (IICESocket::ICESocketState_Shutdown != mRTPSocket->getState()) return false;
        }
        if (mRTCPSocket) {
          if (IICESocket::ICESocketState_Shutdown != mRTCPSocket->getState()) return false;
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

        ICallTransportAsyncProxy::create(outer)->onStep();
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
        ZS_LOG_TRACE(log("request to send RTP packet") + ", payload type=" + Stringize<BYTE>(payloadType).string() + ", length=" + Stringize<int>(len).string())

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
        ZS_LOG_TRACE(log("request to send RTCP packet") + ", payload type=" + Stringize<BYTE>(payloadType).string() + ", length=" + Stringize<int>(len).string())

        CallTransportPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Trace, log("cannot send RTCP packet because call transport object is gone"))
          return 0;
        }

        return outer->sendRTCPPacket(mID, data, len);
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
        return String("CallTransport::TransportSocket [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String CallTransport::TransportSocket::getDebugValueString(bool includeCommaPrefix) const
      {
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("transport socket id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("rtp socket id", mRTPSocket ? Stringize<PUID>(mRTPSocket->getID()).string() : String(), firstTime) +
               Helper::getDebugValue("rtcp socket id", mRTCPSocket ? Stringize<PUID>(mRTCPSocket->getID()).string() : String(), firstTime);
      }
    }
  }
}
