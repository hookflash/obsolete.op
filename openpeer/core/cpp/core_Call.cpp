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

#include <openpeer/core/internal/core_Call.h>
#include <openpeer/core/internal/core_CallTransport.h>
#include <openpeer/core/internal/core_Stack.h>
#include <openpeer/core/internal/core_Account.h>
#include <openpeer/core/internal/core_Contact.h>
#include <openpeer/core/internal/core_Helper.h>
#include <openpeer/stack/message/IMessageHelper.h>

#include <zsLib/helpers.h>

#define OPENPEER_CALL_CLEANUP_TIMEOUT_IN_SECONDS (60*2)

#define OPENPEER_CALL_FIRST_CLOSED_REMOTE_CALL_TIME_IN_SECONDS (4)
#define OPENPEER_CALL_CALL_CHECK_PEER_ALIVE_TIMER_IN_SECONDS (15)

#define OPENPEER_CALL_RTP_ICE_KEEP_ALIVE_INDICATIONS_SENT_IN_SECONDS (4)
#define OPENPEER_CALL_RTP_ICE_EXPECTING_DATA_WITHIN_IN_SECONDS (10)
#define OPENPEER_CALL_RTP_MAX_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS (15)

#define OPENPEER_CALL_RTCP_ICE_KEEP_ALIVE_INDICATIONS_SENT_IN_SECONDS (20)
#define OPENPEER_CALL_RTCP_ICE_EXPECTING_DATA_WITHIN_IN_SECONDS (45)

namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_media) } }

using zsLib::Stringize;

namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      using zsLib::ITimerDelegateProxy;
      using stack::message::IMessageHelper;

      typedef IConversationThreadParser::Dialog Dialog;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static Dialog::DialogStates convert(ICall::CallStates state)
      {
        return (Dialog::DialogStates)state;
      }

      //-----------------------------------------------------------------------
      static ICall::CallStates convert(Dialog::DialogStates state)
      {
        return (ICall::CallStates)state;
      }

      //-----------------------------------------------------------------------
      static Dialog::DialogClosedReasons convert(ICall::CallClosedReasons reason)
      {
        return (Dialog::DialogClosedReasons)reason;
      }

      //-----------------------------------------------------------------------
      static ICall::CallClosedReasons convert(Dialog::DialogClosedReasons reason)
      {
        return (ICall::CallClosedReasons)reason;
      }

      //-----------------------------------------------------------------------
      static ICallTransportForCall::SocketTypes convert(ICallForCallTransport::SocketTypes type)
      {
        switch (type) {
          case ICallForCallTransport::SocketType_Audio: return ICallTransportForCall::SocketType_Audio;
          case ICallForCallTransport::SocketType_Video: return ICallTransportForCall::SocketType_Video;
        }

        ZS_THROW_INVALID_ASSUMPTION("what type of socket is this?")
        return ICallTransportForCall::SocketType_Audio;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      CallPtr ICallForConversationThread::createForIncomingCall(
                                                                ConversationThreadPtr inConversationThread,
                                                                ContactPtr callerContact,
                                                                const DialogPtr &remoteDialog
                                                                )
      {
        return ICallFactory::singleton().createForIncomingCall(inConversationThread, callerContact, remoteDialog);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallForCallTransport
      #pragma mark

      //-----------------------------------------------------------------------
      const char *ICallForCallTransport::toString(SocketTypes type)
      {
        switch (type)
        {
          case SocketType_Audio:  return "audio";
          case SocketType_Video:  return "video";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call::ICallLocation
      #pragma mark

      //-----------------------------------------------------------------------
      const char *Call::ICallLocation::toString(CallLocationStates state)
      {
        switch (state)
        {
          case CallLocationState_Pending:   return "Pending";
          case CallLocationState_Ready:     return "Ready";
          case CallLocationState_Closed:    return "Closed";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call
      #pragma mark

      //-----------------------------------------------------------------------
      Call::Call(
                 AccountPtr account,
                 ConversationThreadPtr conversationThread,
                 ICallDelegatePtr delegate,
                 bool hasAudio,
                 bool hasVideo,
                 const char *callID
                 ) :
        mID(zsLib::createPUID()),
        mQueue(IStackForInternal::queueCore()),
        mDelegate(delegate),
        mCallID(callID ? Stringize<CSTR>(callID).string() : stack::IHelper::randomString(32)),
        mHasAudio(hasAudio),
        mHasVideo(hasVideo),
        mIncomingCall(false),
        mIncomingNotifiedThreadOfPreparing(false),
        mAccount(account),
        mConversationThread(conversationThread),
        mTransport(account->forCall().getCallTransport()),
        mCurrentState(ICall::CallState_None),
        mClosedReason(ICall::CallClosedReason_None),
        mPlaceCall(false),
        mRingCalled(false),
        mAnswerCalled(false),
        mLocalOnHold(false),
        mCreationTime(zsLib::now()),
        mMediaHolding(false),
        mNotifiedCallTransportDestroyed(false)
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void Call::init()
      {
        if (mTransport) {
          mMediaQueue = mTransport->forCall().getMediaQueue();
          mTransport->forCall().notifyCallCreation(mID);
        }
        setCurrentState(ICall::CallState_Preparing);

        ZS_LOG_DEBUG(log("call init called thus invoking step"))
        ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      Call::~Call()
      {
        if (isNoop()) return;

        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      CallPtr Call::convert(ICallPtr call)
      {
        return boost::dynamic_pointer_cast<Call>(call);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => ICall
      #pragma mark

      //-----------------------------------------------------------------------
      String Call::toDebugString(ICallPtr call, bool includeCommaPrefix)
      {
        if (!call) return includeCommaPrefix ? String(", call=(null)") : String("call=(null)");
        return Call::convert(call)->getDebugValueString(true, false, includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      CallPtr Call::placeCall(
                              IConversationThreadPtr inConversationThread,
                              IContactPtr toContact,
                              bool includeAudio,
                              bool includeVideo
                              )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inConversationThread)
        ZS_THROW_INVALID_ARGUMENT_IF(!toContact)

        ConversationThreadPtr conversationThread = ConversationThread::convert(inConversationThread);
        AccountPtr account = conversationThread->forCall().getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, "account object is gone thus cannot create call")
          return CallPtr();
        }

        CallPtr pThis(new Call(account,  conversationThread, account->forCall().getCallDelegate(), includeAudio, includeVideo, NULL));
        pThis->mThisWeak = pThis;
        pThis->mCaller = account->forCall().getSelfContact();
        pThis->mCallee = Contact::convert(toContact);
        pThis->mIncomingCall = false;
        pThis->mPlaceCall = true;
        ZS_LOG_DEBUG(pThis->log("call being placed") + IContact::toDebugString(pThis->mCallee))
        pThis->init();
        if ((!pThis->mCaller) ||
            (!pThis->mCallee)) {
          ZS_LOG_WARNING(Detail, pThis->log("contact is not valid thus cannot create call"))
          return CallPtr();
        }
        if (!pThis->mTransport) {
          ZS_LOG_WARNING(Detail, pThis->log("transport object is gone from account thus cannot create call (while placing call)"))
          return CallPtr();
        }

        return pThis;
      }

      //-----------------------------------------------------------------------
      String Call::getCallID() const
      {
        return mCallID;
      }

      //-----------------------------------------------------------------------
      IConversationThreadPtr Call::getConversationThread() const
      {
        return mConversationThread.lock();
      }

      //-----------------------------------------------------------------------
      IContactPtr Call::getCaller() const
      {
        return mCaller;
      }

      //-----------------------------------------------------------------------
      IContactPtr Call::getCallee() const
      {
        return mCallee;
      }

      //-----------------------------------------------------------------------
      bool Call::hasAudio() const
      {
        return mHasAudio;
      }

      //-----------------------------------------------------------------------
      bool Call::hasVideo() const
      {
        return mHasVideo;
      }

      //-----------------------------------------------------------------------
      ICall::CallStates Call::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      ICall::CallClosedReasons Call::getClosedReason() const
      {
        AutoRecursiveLock lock(getLock());
        return mClosedReason;
      }

      //-----------------------------------------------------------------------
      Time Call::getcreationTime() const
      {
        AutoRecursiveLock lock(getLock());
        return mCreationTime;
      }

      //-----------------------------------------------------------------------
      Time Call::getRingTime() const
      {
        AutoRecursiveLock lock(getLock());
        return mCreationTime;
      }

      //-----------------------------------------------------------------------
      Time Call::getAnswerTime() const
      {
        AutoRecursiveLock lock(getLock());
        return mAnswerTime;
      }

      //-----------------------------------------------------------------------
      Time Call::getClosedTime() const
      {
        AutoRecursiveLock lock(getLock());
        return mClosedTime;
      }

      //-----------------------------------------------------------------------
      void Call::ring()
      {
        ZS_THROW_INVALID_USAGE_IF(!mIncomingCall)

        ZS_LOG_DEBUG(log("ring called"))

        AutoRecursiveLock lock(getLock());
        mRingCalled = true;

        ZS_LOG_DEBUG(log("ring called thus invoking step"))
        ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void Call::answer()
      {
        ZS_THROW_INVALID_USAGE_IF(!mIncomingCall)

        ZS_LOG_DEBUG(log("answer called"))

        AutoRecursiveLock lock(getLock());

        mAnswerCalled = true;

        ZS_LOG_DEBUG(log("answer called thus invoking step"))
        ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void Call::hold(bool hold)
      {
        ZS_LOG_DEBUG(log("hold called"))

        AutoRecursiveLock lock(getLock());
        mLocalOnHold = hold;

        ZS_LOG_DEBUG(log("hold called thus invoking step"))
        ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void Call::hangup(CallClosedReasons reason)
      {
        ZS_LOG_DEBUG(log("hangup called") + ", reason=" + ICall::toString(reason))

        AutoRecursiveLock lock(getLock());
        setClosedReason(reason);
        ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onHangup();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => ICallForCallTransport
      #pragma mark

      //-----------------------------------------------------------------------
      void Call::notifyLostFocus()
      {
        hold(true);
      }

      //-----------------------------------------------------------------------
      bool Call::sendRTPPacket(
                               PUID toLocationID,
                               SocketTypes type,
                               const BYTE *packet,
                               ULONG packetLengthInBytes
                               )
      {
        CallLocationPtr callLocation;

        // scope
        {
          AutoRecursiveLock lock(getMediaLock());
          callLocation = mPickedLocation;

          // NOTE: Intentionally disallow picking of the early location since
          //       local would never send RTP media during early.
        }

        if (!callLocation) {
          ZS_LOG_WARNING(Trace, log("unable to send RTP packet as there is no picked/early location to communicate"))
          return false;
        }
        if (callLocation->getID() != toLocationID) {
          ZS_LOG_WARNING(Trace, log("unable to send RTP packet as the picked/early location does not match the to location"))
          return false;
        }
        return callLocation->sendRTPPacket(type, packet, packetLengthInBytes);
      }

      //-----------------------------------------------------------------------
      bool Call::sendRTCPPacket(
                                PUID toLocationID,
                                SocketTypes type,
                                const BYTE *packet,
                                ULONG packetLengthInBytes
                                )
      {
        CallLocationPtr callLocation;

        // scope
        {
          AutoRecursiveLock lock(getMediaLock());
          callLocation = mPickedLocation;
          if (!callLocation) {
            callLocation = mEarlyLocation;
          }
        }

        if (!callLocation) {
          ZS_LOG_WARNING(Trace, log("unable to send RTCP packet as there is no picked/early location to communicate"))
          return false;
        }
        if (callLocation->getID() != toLocationID) {
          ZS_LOG_WARNING(Trace, log("unable to send the RTCP packet as the picked/early location does not match the to location"))
          return false;
        }
        return callLocation->sendRTCPPacket(type, packet, packetLengthInBytes);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => ICallForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      CallPtr Call::createForIncomingCall(
                                           ConversationThreadPtr conversationThread,
                                           ContactPtr callerContact,
                                           const DialogPtr &remoteDialog
                                           )
      {
        typedef IConversationThreadParser::Dialog::DescriptionList DescriptionList;
        typedef IConversationThreadParser::Dialog::DescriptionPtr DescriptionPtr;

        ZS_THROW_INVALID_ARGUMENT_IF(!conversationThread)
        ZS_THROW_INVALID_ARGUMENT_IF(!callerContact)

        AccountPtr account = conversationThread->forCall().getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, "account object is gone thus cannot create call")
          return CallPtr();
        }

        bool hasAudio = false;
        bool hasVideo = false;

        DescriptionList descriptions = remoteDialog->descriptions();
        for (DescriptionList::iterator iter = descriptions.begin(); iter != descriptions.end(); ++iter)
        {
          DescriptionPtr &description = (*iter);

          bool isAudio = false;
          bool isVideo = false;

          if ("audio" == description->mType) {
            isAudio = true;
            hasAudio = true;
          } else if ("video" == description->mType) {
            isVideo = true;
            hasVideo = true;
          }

          if ((!isAudio) && (!isVideo)) {
            ZS_LOG_WARNING(Detail, String("call does not contain valid media") + ", type=" + description->mType)
            continue;
          }
        }

        CallPtr pThis(new Call(account,  conversationThread, account->forCall().getCallDelegate(), hasAudio, hasVideo, remoteDialog->dialogID()));
        pThis->mThisWeak = pThis;
        pThis->mCaller = callerContact;
        pThis->mCallee = account->forCall().getSelfContact();
        pThis->mIncomingCall = true;
        pThis->init();
        if ((!pThis->mCaller) ||
            (!pThis->mCallee)) {
          ZS_LOG_WARNING(Detail, pThis->log("contact is not valid thus cannot create call"))
          return CallPtr();
        }
        if (!pThis->mTransport) {
          ZS_LOG_WARNING(Detail, pThis->log("transport object is gone from account thus cannot create call (for incoming call)"))
          return CallPtr();
        }

        return pThis;
      }

      //-----------------------------------------------------------------------
      DialogPtr Call::getDialog() const
      {
        AutoRecursiveLock lock(getLock());
        return mDialog;
      }

      //-----------------------------------------------------------------------
      ContactPtr Call::getCaller(bool) const
      {
        return mCaller;
      }

      //-----------------------------------------------------------------------
      ContactPtr Call::getCallee(bool) const
      {
        return mCallee;
      }

      //-----------------------------------------------------------------------
      void Call::notifyConversationThreadUpdated()
      {
        ZS_LOG_DEBUG(log("notified conversation thread updated thus invoking step"))
        ICallAsyncProxy::create(mQueue, mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => ICallAsync
      #pragma mark

      //-----------------------------------------------------------------------
      void Call::onSetFocus(bool on)
      {
        if (!on) {
          ZS_LOG_DEBUG(log("this call should not have focus"))
          mTransport->forCall().loseFocus(mID);
          return;
        }

        PUID focusLocationID = 0;

        {
          AutoRecursiveLock lock(getMediaLock());
          if (mPickedLocation) {
            focusLocationID = mPickedLocation->getID();
          } else if (mEarlyLocation) {
            focusLocationID = mEarlyLocation->getID();
          }
        }

        // tell the transport to focus on this call/location...
        if (0 != focusLocationID) {
          mTransport->forCall().focus(mThisWeak.lock(), focusLocationID);
        } else {
          ZS_LOG_WARNING(Detail, log("told to set focus but there is no location to focus"))
          mTransport->forCall().loseFocus(mID);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => IICESocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Call::onICESocketStateChanged(
                                         IICESocketPtr inSocket,
                                         ICESocketStates state
                                         )
      {
        // scope
        {
          AutoRecursiveLock lock(getMediaLock());

          bool found = false;
          SocketTypes type = SocketType_Audio;
          bool wasRTP = false;

          IICESocketSubscriptionPtr &subscription = findSubscription(inSocket, found, &type, &wasRTP);
          if (!found) {
            ZS_LOG_WARNING(Detail, log("ignoring ICE socket state change on obsolete session"))
            return;
          }

          if (IICESocket::ICESocketState_Shutdown == state) {
            ZS_LOG_DEBUG(log("ICE socket is shutdown") + ", type=" + ICallForCallTransport::toString(type) + ", is RTP=" + (wasRTP ? "true" : "false"))
            subscription.reset();
          }
        }

        ZS_LOG_DEBUG(log("ICE socket state change thus invoking step"))
        ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => ITimerDelegate
      #pragma mark
      void Call::onTimer(TimerPtr timer)
      {
        // scope: check out the timer in the context of a lock
        {
          AutoRecursiveLock lock(getLock());

          if (timer == mPeerAliveTimer) {
            ZS_LOG_DEBUG(log("peer alive timer"))
            goto call_step;
          }

          if (timer == mFirstClosedRemoteCallTimer) {
            ZS_LOG_DEBUG(log("first closed remote call timer"))

            mFirstClosedRemoteCallTimer->cancel();
            mFirstClosedRemoteCallTimer.reset();
            goto call_step;
          }

          if (timer == mCleanupTimer) {
            ZS_LOG_DEBUG(log("call cleanup timer fired - tell the conversation thread"))

            mCleanupTimer.reset();

            ConversationThreadPtr thread = mConversationThread.lock();
            if (!thread) {
              ZS_LOG_WARNING(Debug, log("conversation thread is already shutdown"))
              return;
            }

            thread->forCall().notifyCallCleanup(mThisWeak.lock());
          }
          return;
        }

      call_step:
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => friend CallLocation
      #pragma mark

      //-----------------------------------------------------------------------
      void Call::notifyReceivedRTPPacket(
                                         PUID locationID,
                                         SocketTypes type,
                                         const BYTE *buffer,
                                         ULONG bufferLengthInBytes
                                         )
      {
        // scope:
        {
          AutoRecursiveLock lock(getMediaLock());
          if (!mPickedLocation) {
            if (!mEarlyLocation) {
              ZS_LOG_WARNING(Trace, log("ignoring received RTP packet as no call location was chosen"))
              return;
            }
            if (mEarlyLocation->getID() != locationID) {
              ZS_LOG_WARNING(Trace, log("ignoring received RTP packet as packet did not come from chosen early location"))
              return;
            }
          } else if (mPickedLocation->getID() != locationID) {
            ZS_LOG_WARNING(Trace, log("ignoring received RTP packet as location specified is not chosen location") + ", chosen=" + Stringize<PUID>(mPickedLocation->getID()).string() + ", specified=" + Stringize<PUID>(locationID).string())
            return;
          }
        }

        mTransport->forCall().notifyReceivedRTPPacket(mID, locationID, internal::convert(type), buffer, bufferLengthInBytes);
      }

      //-----------------------------------------------------------------------
      void Call::notifyReceivedRTCPPacket(
                                          PUID locationID,
                                          SocketTypes type,
                                          const BYTE *buffer,
                                          ULONG bufferLengthInBytes
                                          )
      {
        // scope:
        {
          AutoRecursiveLock lock(getMediaLock());
          if (!mPickedLocation) {
            if (!mEarlyLocation) {
              ZS_LOG_WARNING(Trace, log("ignoring received RTCP packet as no call location was chosen"))
              return;
            }
            if (mEarlyLocation->getID() != locationID) {
              ZS_LOG_WARNING(Trace, log("ignoring received RTCP packet as packet did not come from chosen early location"))
              return;
            }
          } else if (mPickedLocation->getID() != locationID) {
            ZS_LOG_WARNING(Trace, log("ignoring received RTCP packet as location specified is not chosen location") + ", chosen=" + Stringize<PUID>(mPickedLocation->getID()).string() + ", specified=" + Stringize<PUID>(locationID).string())
            return;
          }
        }

        mTransport->forCall().notifyReceivedRTCPPacket(mID, locationID, internal::convert(type), buffer, bufferLengthInBytes);
      }

      //-----------------------------------------------------------------------
      void Call::notifyStateChanged(
                                    CallLocationPtr location,
                                    CallLocationStates state
                                    )
      {
        if (ICallLocation::CallLocationState_Closed == state) {
          // scope: object
          {
            AutoRecursiveLock lock(getLock());
            CallLocationMap::iterator found = mCallLocations.find(location->getLocationID());
            if (found != mCallLocations.end()) {
              ZS_LOG_DEBUG(log("location shutdown") + ", object ID=" + Stringize<PUID>(location->getID()).string() + ", location ID=" + location->getLocationID())
              mCallLocations.erase(found);
            }
          }

          bool pickedRemoved = false;

          // scope: media
          {
            AutoRecursiveLock lock(getMediaLock());
            if (mPickedLocation) {
              if (mPickedLocation->getLocationID() == location->getLocationID()) {
                // the picked location is shutting down...
                ZS_LOG_WARNING(Detail, log("picked location shutdown") + ", object ID=" + Stringize<PUID>(location->getID()).string() + ", location ID=" + location->getLocationID())
                mPickedLocation.reset();
                pickedRemoved = true;
              }
            }
          }

          if (pickedRemoved)
          {
            // scope: object
            AutoRecursiveLock lock(getLock());
            setClosedReason(CallClosedReason_RequestTimeout);
          }
        }

        CallPtr pThis = mThisWeak.lock();
        if (pThis) {
          ZS_LOG_DEBUG(log("call location state changed thus invoking step"))
          ICallAsyncProxy::create(getQueue(), pThis)->onStep();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Call::getLock() const
      {
        ConversationThreadPtr thread = mConversationThread.lock();
        if (!thread) return mBogusLock;
        return thread->forCall().getLock();
      }

      //-----------------------------------------------------------------------
      RecursiveLock &Call::getMediaLock() const
      {
        ZS_THROW_INVALID_ASSUMPTION_IF(!mTransport)
        return mTransport->forCall().getLock();
      }

      //-----------------------------------------------------------------------
      RecursiveLock &Call::getStepLock() const
      {
        return mStepLock;
      }

      //-----------------------------------------------------------------------
      String Call::log(const char *message) const
      {
        return String("Call [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String Call::getDebugValueString(
                                       bool callData,
                                       bool mediaData,
                                       bool includeCommaPrefix
                                       ) const
      {
        bool firstTime = !includeCommaPrefix;
        String result = Helper::getDebugValue("call id", Stringize<typeof(mID)>(mID).string(), firstTime) +
                        Helper::getDebugValue("call id (s)", mCallID, firstTime) +
                        Helper::getDebugValue("has audio", mHasAudio ? String("true") : String(), firstTime) +
                        Helper::getDebugValue("has video", mHasVideo ? String("true") : String(), firstTime) +
                        Helper::getDebugValue("incoming", mIncomingCall ? String("true") : String(), firstTime) +
                        ", caller: " + IContact::toDebugString(mCaller, false) +
                        ", callee: " + IContact::toDebugString(mCallee, false);
        if (callData)
        {
          AutoRecursiveLock lock(getLock());
          result += Helper::getDebugValue("state", ICall::toString(mCurrentState), firstTime) +
                    Helper::getDebugValue("closed reason", ICall::toString(mClosedReason), firstTime) +
                    Helper::getDebugValue("notified", mIncomingNotifiedThreadOfPreparing ? String("true") : String(), firstTime) +
                    Helper::getDebugValue("locations", mCallLocations.size() > 0 ? Stringize<size_t>(mCallLocations.size()).string() : String(), firstTime) +
                    Helper::getDebugValue("place call", mPlaceCall ? String("true") : String(), firstTime) +
                    Helper::getDebugValue("ring called", mRingCalled ? String("true") : String(), firstTime) +
                    Helper::getDebugValue("answer called", mAnswerCalled ? String("true") : String(), firstTime) +
                    Helper::getDebugValue("local on hold", mLocalOnHold ? String("true") : String(), firstTime) +
                    Helper::getDebugValue("creation", Time() != mCreationTime ? IMessageHelper::timeToString(mCreationTime) : String(), firstTime) +
                    Helper::getDebugValue("ring", Time() != mRingTime ? IMessageHelper::timeToString(mRingTime) : String(), firstTime) +
                    Helper::getDebugValue("answer", Time() != mAnswerTime ? IMessageHelper::timeToString(mAnswerTime) : String(), firstTime) +
                    Helper::getDebugValue("closed", Time() != mClosedTime ? IMessageHelper::timeToString(mClosedTime) : String(), firstTime) +
                    Helper::getDebugValue("first closed", Time() != mFirstClosedRemoteCallTime ? IMessageHelper::timeToString(mFirstClosedRemoteCallTime) : String(), firstTime);
        }
        if (mediaData)
        {
          AutoRecursiveLock lock(getMediaLock());
          result += Helper::getDebugValue("media hold", mMediaHolding ? String("true") : String(), firstTime) +
                    Helper::getDebugValue("notified destroyed", mNotifiedCallTransportDestroyed ? String("true") : String(), firstTime);
        }
        return result;
      }

      //-----------------------------------------------------------------------
      bool Call::isShuttingdown() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState == ICall::CallState_Closing;
      }

      //-----------------------------------------------------------------------
      bool Call::isShutdown() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState == ICall::CallState_Closed;
      }

      //-----------------------------------------------------------------------
      void Call::cancel()
      {
        typedef zsLib::Timer Timer;
        typedef zsLib::Seconds Seconds;
        typedef std::list<CallLocationPtr> CallLocationList;

        ZS_LOG_DEBUG(log("cancel called"))

        CallLocationList locationsToClose;

        // scope: object
        {
          AutoRecursiveLock lock(getLock());
          if (isShutdown()) {
            ZS_LOG_DEBUG(log("cancel called but call already shutdown"))
            return;
          }

          if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

          setCurrentState(ICall::CallState_Closing);

          for (CallLocationMap::iterator iter = mCallLocations.begin(); iter != mCallLocations.end(); ++iter)
          {
            CallLocationPtr &callLocation = (*iter).second;
            locationsToClose.push_back(callLocation);
          }

          mCallLocations.clear();
        }

        ZS_LOG_DEBUG(log("closing call locations") + ", total=" + Stringize<size_t>(locationsToClose.size()).string())

        // close the locations now...
        for (CallLocationList::iterator iter = locationsToClose.begin(); iter != locationsToClose.end(); ++iter)
        {
          CallLocationPtr &callLocation = (*iter);
          callLocation->close();
        }

        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());

          mTransport->forCall().loseFocus(mID);

          ZS_LOG_DEBUG(log("shutting down audio/video socket subscriptions"))

          if (mAudioRTPSocketSubscription) {
            mAudioRTPSocketSubscription->cancel();
            mAudioRTPSocketSubscription.reset();
          }
          if (mAudioRTCPSocketSubscription) {
            mAudioRTCPSocketSubscription->cancel();
            mAudioRTCPSocketSubscription.reset();
          }
          if (mVideoRTPSocketSubscription) {
            mVideoRTPSocketSubscription->cancel();
            mVideoRTPSocketSubscription.reset();
          }
          if (mVideoRTCPSocketSubscription) {
            mVideoRTCPSocketSubscription->cancel();
            mVideoRTCPSocketSubscription.reset();
          }
        }

        // scope: final object shutdown
        {
          AutoRecursiveLock lock(getLock());

          setCurrentState(ICall::CallState_Closed);

          mDelegate.reset();

          // mDialog.reset(); // DO NOT RESET THIS OBJECT - LEAVE IT ALIVE UNTIL THE CALL OBJECT IS DESTROYED

          if (mGracefulShutdownReference) {
            if (!mCleanupTimer) {
              // We will want the cleanup timer to fire to ensure the
              // conversation thread (which is holding a reference to the call)
              // to forget about this call object in time...
              mCleanupTimer = Timer::create(ITimerDelegateProxy::create(getQueue(), mGracefulShutdownReference), Seconds(OPENPEER_CALL_CLEANUP_TIMEOUT_IN_SECONDS), false);
            }
          }

          if (mPeerAliveTimer) {
            mPeerAliveTimer->cancel();
            mPeerAliveTimer.reset();
          }

          if (mFirstClosedRemoteCallTimer) {
            mFirstClosedRemoteCallTimer->cancel();
            mFirstClosedRemoteCallTimer.reset();
          }

          mGracefulShutdownReference.reset();
        }

        bool notifyDestroyed = false;

        // scope: final media shutdown
        {
          AutoRecursiveLock lock(getMediaLock());

          mPickedLocation.reset();
          mEarlyLocation.reset();

          if (!mNotifiedCallTransportDestroyed) {
            notifyDestroyed = mNotifiedCallTransportDestroyed = true;
          }
        }

        if ((mTransport) &&
            (notifyDestroyed)) {
          mTransport->forCall().notifyCallDestruction(mID);
        }
        // mTransport.reset();  // WARNING: LEAVE FOR THE DESTRUCTOR TO CLEANUP

        ZS_LOG_DEBUG(log("cancel completed"))
      }

      //-----------------------------------------------------------------------
      void Call::checkState(
                            CallStates state,
                            bool isLocal,
                            bool *callClosedNoThrow
                            ) const throw (
                                           Exceptions::IllegalState,
                                           Exceptions::CallClosed
                                           )
      {
        if (callClosedNoThrow)
          *callClosedNoThrow = false;

        bool didPlaceCall = (isLocal ? !mIncomingCall : mIncomingCall);
        ZS_LOG_DEBUG(log("checking state") + ", state=" + ICall::toString(state) + ", placed call=" + (didPlaceCall ? "true" : "false") + ", side=" + (isLocal ? "local" : "remote"))
        switch (state) {
          case ICall::CallState_None:       ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is none") + ", side=" + (isLocal ? "local" : "remote"))
          case ICall::CallState_Preparing:  break;
          case ICall::CallState_Incoming:   if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal") + ", side=" + (isLocal ? "local" : "remote"))} break;
          case ICall::CallState_Placed:     if (!didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal") + ", side=" + (isLocal ? "local" : "remote"))} break;
          case ICall::CallState_Early:      break;
          case ICall::CallState_Ringing:    if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal") + ", side=" + (isLocal ? "local" : "remote"))} break;
          case ICall::CallState_Ringback:   if (!didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal") + ", side=" + (isLocal ? "local" : "remote"))} break;
          case ICall::CallState_Open:       break;
          case ICall::CallState_Active:     break;
          case ICall::CallState_Inactive:   break;
          case ICall::CallState_Hold:       break;
          case ICall::CallState_Closing:
          case ICall::CallState_Closed:     {
            if (callClosedNoThrow) {
              *callClosedNoThrow = true;
            } else {
              ZS_THROW_CUSTOM(Exceptions::CallClosed, log("call state is closed") + ", side=" + (isLocal ? "local" : "remote"))
            }
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void Call::checkLegalWhenNotPicked() const throw (Exceptions::IllegalState)
      {
        bool didPlaceCall = !mIncomingCall;
        ZS_LOG_DEBUG(log("checking state legal when not picked") + ", state=" + ICall::toString(mCurrentState) + ", placed call=" + (didPlaceCall ? "true" : "false") + ", side=local")
        ZS_THROW_CUSTOM_MSG_IF(Exceptions::IllegalState, !didPlaceCall, log("must have picked a location for the local if the call was incoming"))

        switch (mCurrentState) {
          case ICall::CallState_None:       break;  // already handled
          case ICall::CallState_Preparing:  break;  // legal
          case ICall::CallState_Incoming:   break;  // already handled
          case ICall::CallState_Placed:     break;  // legal
          case ICall::CallState_Early:      break;  // legal
          case ICall::CallState_Ringing:    break;  // already handled
          case ICall::CallState_Ringback:   break;  // legal

          case ICall::CallState_Open:
          case ICall::CallState_Active:
          case ICall::CallState_Inactive:
          case ICall::CallState_Hold:
          {
            ZS_THROW_CUSTOM(Exceptions::IllegalState, log("shutting down call as it's in an illegal state - how can call be in answered state if haven't picked a destination yet"))
          }
          case ICall::CallState_Closing:    break;  // already handled
          case ICall::CallState_Closed:     break;  // already handled
        }
      }

      //-----------------------------------------------------------------------
      void Call::checkLegalWhenPicked(
                                      CallStates state,
                                      bool isLocal
                                      ) const throw (Exceptions::IllegalState)
      {
        bool didPlaceCall = (isLocal ? !mIncomingCall : mIncomingCall);
        ZS_LOG_DEBUG(log("checking state is legal when picked") + ", state=" + ICall::toString(state) + ", placed call=" + (didPlaceCall ? "true" : "false") + ", side=" + (isLocal ? "local" : "remote"))
        if (isLocal) {
          switch (state) {
            case ICall::CallState_None:       break;  // already handled
            case ICall::CallState_Preparing:  if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal") + ", side=" + (isLocal ? "local" : "remote"))} break;
            case ICall::CallState_Incoming:   break;
            case ICall::CallState_Placed:     ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - how could local have picked a location but still be placing the call") + ", side=" + (isLocal ? "local" : "remote"))
            case ICall::CallState_Early:      if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - how could local have picked a location but still be in early state") + ", side=" + (isLocal ? "local" : "remote"))} break;
            case ICall::CallState_Ringing:    break;
            case ICall::CallState_Ringback:   if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - how could local have picked a location but still be in ringback state") + ", side=" + (isLocal ? "local" : "remote"))} break;
            case ICall::CallState_Open:       break;
            case ICall::CallState_Active:     break;
            case ICall::CallState_Inactive:   break;
            case ICall::CallState_Hold:       break;
            case ICall::CallState_Closing:    break;  // already handled
            case ICall::CallState_Closed:     break;  // already handled
          }
        } else {
          switch (state) {
            case ICall::CallState_None:       break;  // already handled
            case ICall::CallState_Preparing:  if (!mIncomingCall) { if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - why pick remote if it's still preparing") + ", side=" + (isLocal ? "local" : "remote"))} } break;
            case ICall::CallState_Incoming:   if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - why pick remote if it's still incoming") + ", side=" + (isLocal ? "local" : "remote"))} break;
            case ICall::CallState_Placed:     break;
            case ICall::CallState_Early:      if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - why pick remote if it's still early media") + ", side=" + (isLocal ? "local" : "remote"))} break;
            case ICall::CallState_Ringing:    if (didPlaceCall) {ZS_THROW_CUSTOM(Exceptions::IllegalState, log("call state is illegal - why pick remote if it's still ringing") + ", side=" + (isLocal ? "local" : "remote"))} break;
            case ICall::CallState_Ringback:   break;
            case ICall::CallState_Open:       break;
            case ICall::CallState_Active:     break;
            case ICall::CallState_Inactive:   break;
            case ICall::CallState_Hold:       break;
            case ICall::CallState_Closing:    break;  // already handled
            case ICall::CallState_Closed:     break;  // already handled
          }
        }
      }

      //-----------------------------------------------------------------------
      bool Call::isLockedToAnotherLocation(const DialogPtr &remoteDialog) const
      {
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("unable to determine dialog location locked state as account object is gone"))
          return false;
        }
        String lockedLocationID;
        if (mIncomingCall) {
          // local is the callee and remote is the callee
          lockedLocationID = remoteDialog->calleeLocationID();
        } else {
          // local is the caller and remote is the callee
          lockedLocationID = remoteDialog->callerLocationID();
        }
        if (lockedLocationID.size() < 1) return false;
        return account->forCall().getSelfLocation()->getLocationID() != lockedLocationID;
      }

      //-----------------------------------------------------------------------
      bool Call::stepIsMediaReady(
                                  bool needCandidates,
                                  CandidateList &outAudioRTPCandidates,
                                  CandidateList &outAudioRTCPCandidates,
                                  CandidateList &outVideoRTPCandidates,
                                  CandidateList &outVideoRTCPCandidates
                                  ) throw (Exceptions::StepFailure)
      {
        typedef services::IICESocketDelegateProxy IICESocketDelegateProxy;

        ZS_LOG_DEBUG(log("checking if media is ready"))

        AutoRecursiveLock lock(getMediaLock());

        // setup all the audio ICE socket subscriptions...
        IICESocketPtr socketAudioRTP = mTransport->forCall().getSocket(internal::convert(SocketType_Audio), true);
        IICESocketPtr socketAudioRTCP = mTransport->forCall().getSocket(internal::convert(SocketType_Audio), false);

        IICESocketPtr socketVideoRTP = mTransport->forCall().getSocket(internal::convert(SocketType_Video), true);
        IICESocketPtr socketVideoRTCP = mTransport->forCall().getSocket(internal::convert(SocketType_Video), false);
        if (hasAudio()) {
          if (!mAudioRTPSocketSubscription) {
            ZS_LOG_DEBUG(log("subscripting audio RTP socket"))
            mAudioRTPSocketSubscription = socketAudioRTP->subscribe(IICESocketDelegateProxy::create(mMediaQueue, mThisWeak.lock()));
            ZS_THROW_CUSTOM_IF(Exceptions::StepFailure, !mAudioRTPSocketSubscription)
          }
          if (!mAudioRTCPSocketSubscription) {
            ZS_LOG_DEBUG(log("subscripting audio RTCP socket"))
            mAudioRTCPSocketSubscription = socketAudioRTCP->subscribe(IICESocketDelegateProxy::create(mMediaQueue, mThisWeak.lock()));
            ZS_THROW_CUSTOM_IF(Exceptions::StepFailure, !mAudioRTCPSocketSubscription)
          }
        }

        if (hasVideo()) {
          // setup all the video ICE socket subscriptions...
          if (!mVideoRTPSocketSubscription) {
            ZS_LOG_DEBUG(log("subscripting video RTP socket"))
            mVideoRTPSocketSubscription = socketVideoRTP->subscribe(IICESocketDelegateProxy::create(mMediaQueue, mThisWeak.lock()));
            ZS_THROW_CUSTOM_IF(Exceptions::StepFailure, !mVideoRTPSocketSubscription)
          }
          if (!mVideoRTCPSocketSubscription) {
            ZS_LOG_DEBUG(log("subscripting video RTCP socket"))
            mVideoRTCPSocketSubscription = socketVideoRTCP->subscribe(IICESocketDelegateProxy::create(mMediaQueue, mThisWeak.lock()));
            ZS_THROW_CUSTOM_IF(Exceptions::StepFailure, !mVideoRTCPSocketSubscription)
          }
        }

        if (hasAudio()) {
          socketAudioRTP->wakeup();
          socketAudioRTCP->wakeup();
        }

        if (hasVideo()) {
          socketVideoRTP->wakeup();
          socketVideoRTCP->wakeup();
        }

        if (hasAudio()) {
          if (IICESocket::ICESocketState_Ready != socketAudioRTP->getState()) {
            ZS_LOG_DEBUG(log("waiting for audio RTP socket to wakeup"))
            return false;
          }
          if (IICESocket::ICESocketState_Ready != socketAudioRTCP->getState()) {
            ZS_LOG_DEBUG(log("waiting for audio RTCP socket to wakeup"))
            return false;
          }
        }

        if (hasVideo()) {
          if (IICESocket::ICESocketState_Ready != socketVideoRTP->getState()) {
            ZS_LOG_DEBUG(log("waiting for video RTP socket to wakeup"))
            return false;
          }
          if (IICESocket::ICESocketState_Ready != socketVideoRTCP->getState()) {
            ZS_LOG_DEBUG(log("waiting for video RTCP socket to wakeup"))
            return false;
          }
        }

        ZS_LOG_DEBUG(log("audio and/or video sockets are all awake and ready") + ", has audio=" + (hasAudio() ? "true" : "false") + ", has video=" + (hasVideo() ? "true" : "false"))

        if (needCandidates) {
          ZS_LOG_DEBUG(log("candidates are being fetched"))

          if (hasAudio()) {
            socketAudioRTP->getLocalCandidates(outAudioRTPCandidates);
            socketAudioRTCP->getLocalCandidates(outAudioRTCPCandidates);
          }

          if (hasVideo()) {
            socketVideoRTP->getLocalCandidates(outVideoRTPCandidates);
            socketVideoRTCP->getLocalCandidates(outVideoRTCPCandidates);
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepPrepareCallFirstTime(
                                          CallLocationPtr &picked,
                                          const CandidateList &audioRTPCandidates,
                                          const CandidateList &audioRTCPCandidates,
                                          const CandidateList &videoRTPCandidates,
                                          const CandidateList &videoRTCPCandidates
                                          ) throw (Exceptions::StepFailure)
      {
        typedef Dialog::Description Description;
        typedef Dialog::DescriptionPtr DescriptionPtr;
        typedef Dialog::DescriptionList DescriptionList;

        ZS_LOG_DEBUG(log("preparing first time calls"))

        AutoRecursiveLock lock(getLock());

        if ((isShuttingdown()) ||
            (isShutdown())) {
          ZS_THROW_CUSTOM(Exceptions::StepFailure, log("call unexpectedly shutdown mid step"))
        }

        if (!mDialog) {
          ZS_LOG_DEBUG(log("creating dialog for call"))

          AccountPtr account = mAccount.lock();
          ZS_THROW_CUSTOM_IF(Exceptions::StepFailure, !account)

          setCurrentState(internal::convert(Dialog::DialogState_Preparing));

          DescriptionList descriptions;

          //*******************************************************************
          //*******************************************************************
          //*******************************************************************
          //*******************************************************************
          // HERE - codecs?

          // audio description...
          if (hasAudio()) {
            DescriptionPtr desc = Description::create();
            desc->mVersion = 1;
            desc->mDescriptionID = stack::IHelper::randomString(20);
            desc->mType = "audio";
            desc->mSSRC = 0;
            desc->mCandidateLists[0] = audioRTPCandidates;
            desc->mCandidateLists[1] = audioRTCPCandidates;
            descriptions.push_back(desc);
          }

          // video description...
          if (hasVideo()) {
            DescriptionPtr desc = Description::create();
            desc->mVersion = 1;
            desc->mDescriptionID = stack::IHelper::randomString(20);
            desc->mType = "video";
            desc->mSSRC = 0;
            desc->mCandidateLists[0] = videoRTPCandidates;
            desc->mCandidateLists[1] = videoRTCPCandidates;
            descriptions.push_back(desc);
          }

          String remoteLocationID;
          if (picked) {
            remoteLocationID = picked->getLocationID();
          }

          mDialog = Dialog::create(
                                   1,
                                   mCallID,
                                   Dialog::DialogState_Preparing,
                                   internal::convert(CallClosedReason_None),
                                   mCaller->forCall().getPeerURI(),
                                   (mIncomingCall ? remoteLocationID : account->forCall().getSelfLocation()->getLocationID()),
                                   mCallee->forCall().getPeerURI(),
                                   (mIncomingCall ? account->forCall().getSelfLocation()->getLocationID() : remoteLocationID),
                                   NULL,
                                   descriptions,
                                   account->forCall().getPeerFiles()
                                   );

          ZS_LOG_DEBUG(log("dialog for call created"))
        }

        if (mPlaceCall) {
          ZS_THROW_CUSTOM_IF(Exceptions::StepFailure, !placeCallWithConversationThread())
          mPlaceCall = false;
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepPrepareCallLocations(
                                          CallLocationPtr &picked,
                                          const LocationDialogMap locationDialogMap,
                                          CallLocationList &outLocationsToClose
                                          ) throw (Exceptions::CallClosed)
      {
        ZS_LOG_DEBUG(log("preparing call locations"))

        Time tick = zsLib::now();

        bool foundPicked = false;
        if (picked) {
          for (LocationDialogMap::const_iterator iter = locationDialogMap.begin(); iter != locationDialogMap.end(); ++iter)
          {
            const String &locationID = (*iter).first;
            if (locationID == picked->getLocationID()) {
              foundPicked = true;
              break;
            }
          }
        }

        if (picked) {
          if (!foundPicked) {
            setClosedReason(CallClosedReason_ServerInternalError);
            ZS_THROW_CUSTOM(Exceptions::CallClosed, log("picked location is now gone"))
          }
          if (mFirstClosedRemoteCallTimer) {
            ZS_LOG_DEBUG(log("cancelling unneeded timer"))
            mFirstClosedRemoteCallTimer->cancel();
            mFirstClosedRemoteCallTimer.reset();
          }
        }

        CallClosedReasons lastClosedReason = CallClosedReason_None;

        for (LocationDialogMap::const_iterator iter = locationDialogMap.begin(); iter != locationDialogMap.end(); ++iter)
        {
          const String &locationID = (*iter).first;
          const DialogPtr &remoteDialog = (*iter).second;

          CallLocationMap::iterator found = mCallLocations.find(locationID);
          if (found != mCallLocations.end()) {
            ZS_LOG_DEBUG(log("updating an existing call location") + ", remote location ID=" + locationID)
            CallLocationPtr &callLocation = (*found).second;
            callLocation->updateDialog(remoteDialog);

            if (picked) {
              if (locationID != picked->getLocationID()) {
                ZS_LOG_DEBUG(log("this call location was not picked therefor must close later") + ", remote: " + CallLocation::toDebugString(callLocation, true, false, false) + ", picked: " + CallLocation::toDebugString(picked, true, false, false))
                // this location must be removed...
                CallLocationPtr &callLocation = (*found).second;
                outLocationsToClose.push_back(callLocation);

                // we don't want this call location anymore since we have picked something else...
                mCallLocations.erase(found);
              }
            }
            continue;
          }

          if (picked) {
            ZS_LOG_DEBUG(log("found a new location for the call but already picked the current location") + ", location ID=" + locationID)
            continue;
          }

          CallStates remoteCallState = internal::convert(remoteDialog->dialogState());

          if ((remoteCallState != ICall::CallState_Closing) &&
              (remoteCallState != ICall::CallState_Closed)) {

            ZS_LOG_DEBUG(log("creating a new call location") + ", remote location ID=" + locationID)
            CallLocationPtr callLocation = CallLocation::create(getQueue(), getMediaQueue(), mThisWeak.lock(), mTransport, locationID, remoteDialog, hasAudio(), hasVideo());
            mCallLocations[locationID] = callLocation;

            if (mIncomingCall) {
              ZS_LOG_DEBUG(log("incoming call must pick the remote location") + ", remote local ID=" + locationID + CallLocation::toDebugString(callLocation, true, false))
              // we *must* pick this location
              picked = callLocation;
            }
            continue;
          }

          CallClosedReasons remoteReason = internal::convert(remoteDialog->closedReason());
          ZS_LOG_DEBUG(log("found a new location for the call but location was already closing/closed") + ", location ID=" + locationID + ", reason=" + ICall::toString(remoteReason))

          switch (internal::convert(remoteDialog->closedReason())) {
            case CallClosedReason_Decline: {
              ZS_LOG_DETAIL(log("call refused by remote party (must end call now)"))
              setClosedReason(internal::convert(remoteDialog->closedReason()));
              return false;
            }
            default:  {
              lastClosedReason = (CallClosedReason_None != remoteReason ? remoteReason : CallClosedReason_User);

              if (Time() == mFirstClosedRemoteCallTime) {
                mFirstClosedRemoteCallTime = tick;
              }
              break;
            }
          }
        }

        if (mIncomingCall) {
          if ((!picked) &&
              (CallClosedReason_None != lastClosedReason)) {
            mFirstClosedRemoteCallTime = Time();
            setClosedReason(lastClosedReason);
            return false;
          }
        }

        //...................................................................
        // check to see if remote dialogs were closed without having any
        // that did not reject the location... if so set up a timer
        // to reject the call in a reasonable time frame

        if (!picked) {
          if (mCallLocations.size() < 1) {
            if (CallClosedReason_None != lastClosedReason) {
              ZS_LOG_DEBUG(log("did not find any remote locations that were open (but one or more were closed)"))

              if (mFirstClosedRemoteCallTime + Seconds(OPENPEER_CALL_FIRST_CLOSED_REMOTE_CALL_TIME_IN_SECONDS) < tick) {
                setClosedReason(lastClosedReason);
                return false;
              }
              if (!mFirstClosedRemoteCallTimer) {
                mFirstClosedRemoteCallTimer = Timer::create(ITimerDelegateProxy::createWeak(getQueue(), mThisWeak.lock()), Seconds(OPENPEER_CALL_FIRST_CLOSED_REMOTE_CALL_TIME_IN_SECONDS), false);
              }
              ZS_LOG_DEBUG(log("remote party is closed but will try to connect with other locations (if possible)"))
            }
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepVerifyCallState(CallLocationPtr &picked) throw (
                                                                     Exceptions::IllegalState,
                                                                     Exceptions::CallClosed
                                                                     )
      {
        ZS_LOG_DEBUG(log("verifying call state"))

        if ((mIncomingCall) && (!picked)) {
          setClosedReason(CallClosedReason_ServerInternalError);
          ZS_THROW_CUSTOM(Exceptions::CallClosed, log("shutting down call as incoming call does not have a location picked and it must"))
        }

        bool callClosedException = false;
        checkState(mCurrentState, true, &callClosedException);
        if (callClosedException) {
          ZS_LOG_DEBUG(log("local side closed the call"))
          setClosedReason(CallClosedReason_User);
          return false;
        }

        if (picked) {
          DialogPtr pickedRemoteDialog = picked->getRemoteDialog();
          ZS_THROW_INVALID_ASSUMPTION_IF(!pickedRemoteDialog)
          checkState(internal::convert(pickedRemoteDialog->dialogState()), false, &callClosedException);
          if (callClosedException) {
            ZS_LOG_DEBUG(log("remote side closed the call"))
            setClosedReason(internal::convert(pickedRemoteDialog->closedReason()));
            return false;
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepTryToPickALocation(
                                        AccountPtr &account,
                                        CallLocationPtr &ioEarly,
                                        CallLocationPtr &ioPicked,
                                        CallLocationList &outLocationsToClose
                                        )
      {
        typedef Dialog::Description Description;
        typedef Dialog::DescriptionList DescriptionList;

        if (ioPicked) {
          DialogPtr remoteDialog = ioPicked->getRemoteDialog();
          if (isLockedToAnotherLocation(remoteDialog)) {
            ZS_LOG_WARNING(Detail, log("remote side locked onto another location... harumph... remove the location") + Dialog::toDebugString(remoteDialog))
            return false;
          }

          ZS_LOG_DEBUG(log("location already picked (no need to try and pick one)"))
          return true;
        }

        ZS_LOG_DEBUG(log("do not have a picked location yet for placed call (thus will attempt to pick one)"))

        checkLegalWhenNotPicked();

        CallLocationPtr ready;
        CallLocationPtr ringing;

        // check for a location which has answered or open the call so we can pick it...
        for (CallLocationMap::iterator locIter = mCallLocations.begin(); locIter != mCallLocations.end(); )
        {
          CallLocationMap::iterator current = locIter;
          ++locIter;

          CallLocationPtr &callLocation = (*current).second;
          DialogPtr remoteDialog = callLocation->getRemoteDialog();

          bool mustRemove = false;

          if (isLockedToAnotherLocation(remoteDialog)) {
            ZS_LOG_WARNING(Detail, log("remote side stupidly locked onto another location which did not even place this call - removing the stupid location"))
            mustRemove = true;
          }

          try {
            ZS_LOG_TRACE(log("checking possible location to see if it has a valid state") + CallLocation::toDebugString(callLocation, true, false) + Dialog::toDebugString(remoteDialog))
            bool callClosedException = false;
            checkState(internal::convert(remoteDialog->dialogState()), false, &callClosedException);
            if (callClosedException) {
              ZS_LOG_WARNING(Detail, log("this non picked location closed the call"))
              mustRemove = true;
            }
          } catch (Exceptions::IllegalState &) {
            ZS_LOG_DEBUG(log("remote call is not in a legal state"))
            mustRemove = true;
          } catch (Exceptions::CallClosed &) {
            ZS_LOG_DEBUG(log("remote call is closed"))
            mustRemove = true;
          }

          if (mustRemove) {
            // this location must close
            outLocationsToClose.push_back(callLocation);
            mCallLocations.erase(current);
            continue;
          }

          if (ICallLocation::CallLocationState_Ready == callLocation->getState()) {
            if (!ready) {
              ready = callLocation;
            }
          }

          switch (internal::convert(remoteDialog->dialogState()))
          {
            case CallState_Early:     {
              if (ICallLocation::CallLocationState_Ready == callLocation->getState()) {
                if (!ioEarly) {
                  ioEarly = callLocation;
                }
              }
              break;
            }
            case CallState_Ringing:   {
              if (!ringing) {
                ringing = callLocation;
              }
              break;
            }
            case CallState_Open:
            case CallState_Active:
            case CallState_Inactive:
            case CallState_Hold:      {
              if (ICallLocation::CallLocationState_Ready == callLocation->getState()) {
                ZS_LOG_DEBUG(log("picked location") + CallLocation::toDebugString(callLocation, true, false))
                ioPicked = callLocation;
              }
              break;
            }
            default: break;
          }

          if (ioPicked) {
            break;
          }
        }

        if (ioPicked) {
          ZS_LOG_DEBUG(log("placed call now has chosen a remote location thus must update dialog"))

          DialogPtr remoteDialog = ioPicked->getRemoteDialog();
          ZS_THROW_CUSTOM_MSG_IF(Exceptions::CallClosed, isLockedToAnotherLocation(remoteDialog), log("remote side has locked onto another location thus call must close"))

          DescriptionList descriptions = mDialog->descriptions();
          mDialog = Dialog::create(
                                   mDialog->version() + 1,
                                   mCallID,
                                   internal::convert(CallState_Open),
                                   internal::convert(CallClosedReason_None),
                                   mCaller->forCall().getPeerURI(),
                                   (mIncomingCall ? ioPicked->getLocationID() : account->forCall().getSelfLocation()->getLocationID()),
                                   mCallee->forCall().getPeerURI(),
                                   (mIncomingCall ? account->forCall().getSelfLocation()->getLocationID() : ioPicked->getLocationID()),
                                   NULL,
                                   descriptions,
                                   account->forCall().getPeerFiles()
                                   );

          setCurrentState(CallState_Open);

          ZS_LOG_DEBUG(log("call state changed to open thus forcing step to force close unchosen locations"))
          ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
        } else if (ioEarly) {
          setCurrentState(CallState_Early);
        } else if (ringing) {
          // force our side into correct state...
          setCurrentState(CallState_Ringback);
        } else if (ready) {
          setCurrentState(CallState_Placed);
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepHandlePickedLocation(
                                          bool &ioMediaHolding,
                                          CallLocationPtr &picked
                                          ) throw (Exceptions::IllegalState)
      {
        if (!picked) {
          ZS_LOG_DEBUG(log("location is not picked yet"))
          return true;
        }

        DialogPtr pickedRemoteDialog = picked->getRemoteDialog();
        ZS_THROW_BAD_STATE_IF(!pickedRemoteDialog)

        ICallLocation::CallLocationStates pickedLocationState = picked->getState();

        ZS_LOG_DEBUG(log("picked call state logic") + CallLocation::toDebugString(picked, true, false) + getDebugValueString(true, false) + Dialog::toDebugString(pickedRemoteDialog))

        // if the picked location is now gone then we must shutdown the object...
        checkLegalWhenPicked(mCurrentState, true);
        checkLegalWhenPicked(internal::convert(pickedRemoteDialog->dialogState()), false);

        switch (pickedLocationState) {
          case ICallLocation::CallLocationState_Pending: {
            ioMediaHolding = true;
            ZS_THROW_CUSTOM_MSG_IF(Exceptions::IllegalState, !mIncomingCall, log("placed call picked location but picked location is not ready"))
            if (mIncomingCall) {
              if (!mIncomingNotifiedThreadOfPreparing) {
                if (ICall::CallState_Preparing == mCurrentState) {
                  ZS_LOG_DEBUG(log("need to notify the remote party that the call is pending since this is an incoming call..."))
                  mIncomingNotifiedThreadOfPreparing = true;
                  setCurrentState(ICall::CallState_Preparing, true);
                }
              }
            }
            break;
          }
          case ICallLocation::CallLocationState_Ready: {
            if (!mIncomingCall) {
              ZS_LOG_DEBUG(log("now ready to go into open state"))
              ioMediaHolding = mLocalOnHold || (ICall::CallState_Hold == internal::convert(pickedRemoteDialog->dialogState()));
              setCurrentState(ICall::CallState_Open);
              break;
            }

            // this call is incoming... answer takes priority over ringing...
            if (mAnswerCalled) {
              ioMediaHolding = mLocalOnHold || (ICall::CallState_Hold == internal::convert(pickedRemoteDialog->dialogState()));
              setCurrentState(ICall::CallState_Open);
            } else if (mRingCalled) {
              ioMediaHolding = true;
              setCurrentState(ICall::CallState_Ringing);
            } else {
              ioMediaHolding = true;
              setCurrentState(ICall::CallState_Incoming);
            }
            break;
          }
          case ICallLocation::CallLocationState_Closed: {
            ZS_LOG_DEBUG(log("picked call location is now closed"))
            setClosedReason(internal::convert(pickedRemoteDialog->closedReason()));
            return false;
          }
        }

        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepFixCallInProgressStates(
                                             AccountPtr &account,
                                             bool mediaHolding,
                                             CallLocationPtr &early,
                                             CallLocationPtr &picked
                                             )
      {
        if ((picked) ||
            (early)) {

          CallLocationPtr usingLocation = (picked ? picked : early);

          ContactPtr remoteContact = (mCaller->forCall().isSelf() ? mCallee : mCaller);
          ZS_THROW_BAD_STATE_IF(remoteContact->forCall().isSelf())

          stack::IAccountPtr stackAccount = account->forCall().getStackAccount();
          if (!stackAccount) {
            ZS_LOG_WARNING(Detail, log("stack account is gone"))
            return false;
          }

          ILocationPtr peerLocation = ILocation::getForPeer(remoteContact->forCall().getPeer(), usingLocation->getLocationID());
          if (!peerLocation) {
            ZS_LOG_WARNING(Detail, log("peer location is not known by stack"))
            return false;
          }

          if (!(peerLocation->isConnected())) {
            ZS_LOG_WARNING(Detail, log("peer location is not known by stack"))
            return false;
          }

          if (!mPeerAliveTimer) {
            ZS_LOG_DEBUG(log("peer alive timer is required (thus starting the timer)"))
            mPeerAliveTimer = Timer::create(ITimerDelegateProxy::createWeak(getQueue(), mThisWeak.lock()), Seconds(OPENPEER_CALL_CALL_CHECK_PEER_ALIVE_TIMER_IN_SECONDS));
          }

          if (ICall::CallState_Hold != mCurrentState) {
            // this call must be in focus...
            ZS_LOG_DEBUG(log("setting focus to this call?") + ", focus=" + (mediaHolding ? "false" : "true"))
            ICallAsyncProxy::create(getMediaQueue(), mThisWeak.lock())->onSetFocus(!mediaHolding);
          } else {
            ZS_LOG_DEBUG(log("this call state is holding thus it should not have focus"))
            ICallAsyncProxy::create(getMediaQueue(), mThisWeak.lock())->onSetFocus(false);
          }

          return true;
        }

        ZS_LOG_DEBUG(log("this call does not have any picked remote locatio thus it should not have focus"))
        ICallAsyncProxy::create(getMediaQueue(), mThisWeak.lock())->onSetFocus(false);

        if (mPeerAliveTimer) {
          ZS_LOG_DEBUG(log("peer alive timer is not required (thus stopping the timer)"))
          mPeerAliveTimer->cancel();
          mPeerAliveTimer.reset();
        }
        return true;
      }

      //-----------------------------------------------------------------------
      bool Call::stepCloseLocations(CallLocationList &locationsToClose)
      {
        if (locationsToClose.size() > 0) {
          ZS_LOG_DEBUG(log("since locations were closed we must invoke a step to cleanup properly"))
          ICallAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
        }

        // force a closing of all these call locations...
        for (CallLocationList::iterator iter = locationsToClose.begin(); iter != locationsToClose.end(); ++iter)
        {
          CallLocationPtr &callLocation = (*iter);
          ZS_LOG_DEBUG(log("forcing call location to close") + CallLocation::toDebugString(callLocation, true, false))
          callLocation->close();
        }
        return true;
      }

      //-----------------------------------------------------------------------
      void Call::step()
      {
        typedef Dialog::DialogStates DialogStates;

        AutoRecursiveLock lock(getStepLock());

        CallLocationPtr picked;
        CallLocationPtr early;
        bool mediaHolding = false;
        bool needCandidates = false;

        CandidateList audioRTPCandidates;
        CandidateList audioRTCPCandidates;
        CandidateList videoRTPCandidates;
        CandidateList videoRTCPCandidates;

        CallLocationList locationsToClose;
        LocationDialogMap locationDialogMap;
        ConversationThreadPtr thread = mConversationThread.lock();

        // scope: object
        {
          AutoRecursiveLock lock(getLock());
          if ((isShuttingdown()) ||
              (isShutdown())) {
            ZS_LOG_DEBUG(log("step called but call is shutting down instead thus redirecting to cancel()"))
            cancel();
            return;
          }

          if (CallClosedReason_None != mClosedReason) {
            ZS_LOG_WARNING(Detail, log("call close reason set thus call must be shutdown") + ", reason=" + ICall::toString(mClosedReason))
            goto call_closed_exception;
          }

          needCandidates = !mDialog;
        }

        ZS_LOG_DEBUG(log("step") + getDebugValueString(true, true))

        // scope: media
        try
        {
          if (!stepIsMediaReady(needCandidates, audioRTPCandidates, audioRTCPCandidates, videoRTPCandidates, videoRTCPCandidates)) {
            ZS_LOG_DEBUG(log("waiting for media to be ready"))
            return;
          }

          AutoRecursiveLock lock(getMediaLock());

          picked = mPickedLocation;
          early = mEarlyLocation;
          mediaHolding = mMediaHolding;

        } catch(Exceptions::StepFailure &) {
          ZS_LOG_WARNING(Detail, log("failed to properly setup call object during media setup (thus shutting down)"))
          setClosedReason(CallClosedReason_ServerInternalError);
          goto call_closed_exception;
        }

        // scope: object
        try
        {
          if (!stepPrepareCallFirstTime(picked, audioRTPCandidates, audioRTCPCandidates, videoRTPCandidates, videoRTCPCandidates)) {
            ZS_LOG_DEBUG(log("prepare first call caused call to close"))
            goto call_closed_exception;
          }
        } catch(Exceptions::StepFailure &) {
          ZS_LOG_WARNING(Detail, log("failed to properly setup call object during object setup (thus shutting down)"))
          setClosedReason(CallClosedReason_ServerInternalError);
          goto call_closed_exception;
        }

        ZS_LOG_DEBUG(log("gathering dialog replies"))

        // examine what is going on in the conversation thread...
        thread->forCall().gatherDialogReplies(mCallID, locationDialogMap);

        ZS_LOG_DEBUG(log("gathering dialog replies has completed") + ", total found=" + Stringize<size_t>(locationDialogMap.size()).string())

        try
        {
          AutoRecursiveLock lock(getLock());
          if ((isShuttingdown()) ||
              (isShutdown())) {
            ZS_THROW_CUSTOM(Exceptions::StepFailure, log("call unexpectedly shutdown mid step"))
          }

          AccountPtr account = mAccount.lock();
          if (!account) {
            setClosedReason(CallClosedReason_ServerInternalError);
            ZS_THROW_CUSTOM(Exceptions::CallClosed, log("acount is now gone thus call must close"))
          }

          if (!stepPrepareCallLocations(picked, locationDialogMap, locationsToClose)) {
            ZS_LOG_DEBUG(log("preparing call locations caused call to close"))
            goto call_closed_exception;
          }

          if (!stepVerifyCallState(picked)) {
            ZS_LOG_DEBUG(log("verifying call state caused call to close"))
            goto call_closed_exception;
          }

          if (!stepTryToPickALocation(account, early, picked, locationsToClose)) {
            ZS_LOG_DEBUG(log("trying to pick a location caused the call to close"))
            goto call_closed_exception;
          }

          if (!stepHandlePickedLocation(mediaHolding, picked)) {
            ZS_LOG_DEBUG(log("handling picked call location caused call to close"))
            goto call_closed_exception;
          }

          if (!stepFixCallInProgressStates(account, mediaHolding, early, picked)) {
            ZS_LOG_DEBUG(log("fix call in progress state caused call to close"))
            goto call_closed_exception;
          }

        } catch(Exceptions::StepFailure &) {
          ZS_LOG_WARNING(Debug, log("failed to properly setup call object during media setup thus shutting down"))
          setClosedReason(CallClosedReason_ServerInternalError);
          goto call_closed_exception;
        } catch(Exceptions::IllegalState &) {
          ZS_LOG_WARNING(Debug, log("call was not in legal state thus shutting down"))
          setClosedReason(CallClosedReason_ServerInternalError);
          goto call_closed_exception;
        } catch(Exceptions::CallClosed &) {
          ZS_LOG_WARNING(Debug, log("call was closed thus shutting down"))
          setClosedReason(CallClosedReason_User);
          goto call_closed_exception;
        }


        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());
          if (mediaHolding != mMediaHolding) {
            ZS_LOG_DEBUG(log("changing media holding state") + ", now holding=" + (mediaHolding ? "true" : "false"))
            mMediaHolding = mediaHolding;
          }
          if ((early) && (!mEarlyLocation)) {
            mEarlyLocation = early;
          }
          if ((picked) && (!mPickedLocation)) {
            ZS_LOG_DEBUG(log("this call now has a picked location") + CallLocation::toDebugString(picked, false, true))
            mPickedLocation = picked;
            mEarlyLocation.reset();
          }
        }

        if (!stepCloseLocations(locationsToClose)) {
          ZS_LOG_DEBUG(log("closing locations caused the call to close"))
          goto call_closed_exception;
        }

        ZS_LOG_DEBUG(log("step completed") + getDebugValueString(true, true))
        return;

      call_closed_exception:
        {
          ZS_LOG_WARNING(Debug, log("call was closed thus shutting down (likely be normal hangup/refuse call)"))
          setClosedReason(CallClosedReason_User);
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void Call::setCurrentState(
                                 CallStates state,
                                 bool forceOverride
                                 )
      {
        typedef Dialog::DescriptionList DescriptionList;
        typedef Dialog::DialogStates DialogStates;

        AutoRecursiveLock lock(getLock());

        if (mLocalOnHold) {
          switch (state) {
            case CallState_Open:
            case CallState_Active:
            case CallState_Inactive:
            case CallState_Hold:
            {
              ZS_LOG_DEBUG(log("call state is being forced to be treated as 'hold' state") + ", was state=" + ICall::toString(state))
              state = CallState_Hold;
              break;
            }
            default:  break;
          }
        }

        if (!forceOverride) {
          if (state == mCurrentState) return;
        }

        bool changedState = false;
        if (state != mCurrentState) {
          ZS_LOG_BASIC(log("state changed") + ", new state=" + ICall::toString(state) + getDebugValueString(true, false))
          mCurrentState = state;
          changedState = true;
        } else {
          ZS_LOG_DEBUG(log("forcing call to state change") + ", state=" + ICall::toString(state) + getDebugValueString(true, false))
        }

        // record when certain states changed...
        switch (mCurrentState)
        {
          case CallState_None:
          case CallState_Preparing:
          case CallState_Incoming:
          case CallState_Placed:
          case CallState_Early:       break;
          case CallState_Ringing:
          case CallState_Ringback:
          {
            if (Time() == mRingTime) {
              mRingTime = zsLib::now();
            }
            break;
          }
          case CallState_Open:
          case CallState_Active:
          case CallState_Inactive:
          case CallState_Hold:
          {
            if (Time() == mRingTime) {
              mRingTime = zsLib::now();
            }
            if (Time() == mAnswerTime) {
              mAnswerTime = zsLib::now();
            }
            break;
          }
          case CallState_Closing:
          case CallState_Closed:
          {
            if (Time() == mClosedTime) {
              mClosedTime = zsLib::now();
            }
            break;
          }
        }

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone thus object cannot notify of state change"))
          return;
        }

        if (mDialog) {
          ZS_LOG_DEBUG(log("creating replacement dialog state"))

          DescriptionList descriptions = mDialog->descriptions();
          mDialog = Dialog::create(
                                   mDialog->version() + 1,
                                   mCallID,
                                   internal::convert(state),
                                   CallState_Closed == state ? internal::convert(mClosedReason) : internal::convert(CallClosedReason_None),
                                   mCaller->forCall().getPeerURI(),
                                   (mIncomingCall ? mDialog->callerLocationID() : account->forCall().getSelfLocation()->getLocationID()),
                                   mCallee->forCall().getPeerURI(),
                                   (mIncomingCall ? account->forCall().getSelfLocation()->getLocationID() : mDialog->calleeLocationID()),
                                   NULL,
                                   descriptions,
                                   account->forCall().getPeerFiles()
                                   );
        }

        CallPtr pThis = mThisWeak.lock();
        if (pThis) {
          if (changedState) {
            ZS_LOG_DEBUG(log("notifying delegate of state change") + ", state=" + ICall::toString(state) + getDebugValueString(true, false))
            try {
              mDelegate->onCallStateChanged(pThis, state);
            } catch (ICallDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone"))
              mDelegate.reset();
            }
          }

          ConversationThreadPtr thread = mConversationThread.lock();
          if (thread) {
            ZS_LOG_DEBUG(log("notifying conversation thread of state change") + ", state=" + ICall::toString(state) + getDebugValueString(true, false))
            thread->forCall().notifyCallStateChanged(pThis);
          }
        }
      }

      //-----------------------------------------------------------------------
      void Call::setClosedReason(CallClosedReasons reason)
      {
        AutoRecursiveLock lock(getLock());
        if (CallClosedReason_None == reason) {
          ZS_LOG_DEBUG(log("cannot set call closed reason to none"))
          return;
        }
        if (CallClosedReason_None != mClosedReason) {
          ZS_LOG_WARNING(Debug, log("once the call closed reason is set it cannot be changed (probably okay)") + ", current reason=" + ICall::toString(mClosedReason) + ", attempted to change to=" + ICall::toString(reason))
          return;
        }
        ZS_LOG_DEBUG(log("call closed reason is now set") + ", reason=" + ICall::toString(reason))
        mClosedReason = reason;
      }

      //-----------------------------------------------------------------------
      IICESocketSubscriptionPtr &Call::findSubscription(
                                                        IICESocketPtr socket,
                                                        bool &outFound,
                                                        SocketTypes *outType,
                                                        bool *outIsRTP
                                                        )
      {
        static IICESocketSubscriptionPtr bogus;

        outFound = true;
        if (outType) *outType = SocketType_Audio;
        if (outIsRTP) *outIsRTP = true;

        if (!socket) {
          ZS_LOG_WARNING(Detail, log("received request to find a NULL socket"))
          return bogus;
        }

        if (hasAudio()) {
          if (socket == mTransport->forCall().getSocket(internal::convert(SocketType_Audio), true)) {
            return mAudioRTPSocketSubscription;
          }
          if (socket == mTransport->forCall().getSocket(internal::convert(SocketType_Audio), false)) {
            if (outIsRTP) *outIsRTP = false;
            return mAudioRTCPSocketSubscription;
          }
        }

        if (outType) *outType = SocketType_Video;
        if (hasVideo()) {
          if (socket == mTransport->forCall().getSocket(internal::convert(SocketType_Video), true)) {
            return mVideoRTPSocketSubscription;
          }
          if (socket == mTransport->forCall().getSocket(internal::convert(SocketType_Video), false)) {
            if (outIsRTP) *outIsRTP = false;
            return mVideoRTCPSocketSubscription;
          }
        }

        ZS_LOG_WARNING(Detail, log("did not find socket subscription for socket") + ", socket ID=" + Stringize<PUID>(socket->getID()).string())

        outFound = false;
        if (outType) *outType = SocketType_Audio;
        if (outIsRTP) *outIsRTP = false;
        return bogus;
      }

      //-----------------------------------------------------------------------
      bool Call::placeCallWithConversationThread()
      {
        ConversationThreadPtr thread = mConversationThread.lock();
        if (!thread) {
          ZS_LOG_WARNING(Detail, log("unable to place call as conversation thread object is gone"))
          return false;
        }
        return thread->forCall().placeCall(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call::CallLocation
      #pragma mark

      //-----------------------------------------------------------------------
      Call::CallLocation::CallLocation(
                                       IMessageQueuePtr queue,
                                       IMessageQueuePtr mediaQueue,
                                       CallPtr outer,
                                       const char *locationID,
                                       const DialogPtr &remoteDialog,
                                       bool hasAudio,
                                       bool hasVideo
                                       ) :
        mQueue(queue),
        mMediaQueue(mediaQueue),
        mID(zsLib::createPUID()),
        mOuter(outer),
        mLocationID(locationID ? String(locationID) : String()),
        mCurrentState(CallLocationState_Pending),
        mRemoteDialog(remoteDialog),
        mHasAudio(hasAudio),
        mHasVideo(hasVideo)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!remoteDialog)
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::init(CallTransportPtr transport)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!transport)

        typedef IICESocket::CandidateList CandidateList;
        typedef IConversationThreadParser::Dialog::DescriptionList DescriptionList;
        typedef IConversationThreadParser::Dialog::DescriptionPtr DescriptionPtr;
        typedef IConversationThreadParser::CandidateLists CandidateLists;

        CallPtr outer = mOuter.lock();
        ZS_THROW_INVALID_ASSUMPTION_IF(!outer)

        DescriptionList descriptions = mRemoteDialog->descriptions();
        for (DescriptionList::iterator iter = descriptions.begin(); iter != descriptions.end(); ++iter)
        {
          DescriptionPtr &description = (*iter);

          CandidateLists::iterator foundRTP = description->mCandidateLists.find(0);
          CandidateLists::iterator foundRTCP = description->mCandidateLists.find(1);
          if (foundRTP == description->mCandidateLists.end()) {
            ZS_LOG_WARNING(Detail, log("missing candidate foundation for RTP"))
            continue;
          }
          if (foundRTCP == description->mCandidateLists.end()) {
            ZS_LOG_WARNING(Detail, log("missing candidate foundation for RTCP"))
            continue;
          }

          CandidateList &rtpCandidates = (*foundRTP).second;
          CandidateList &rtcpCandidates = (*foundRTCP).second;

          bool isAudio = false;
          bool isVideo = false;
          ICallTransportForCall::SocketTypes type = ICallTransportForCall::SocketType_Audio;

          if ("audio" == description->mType) {
            if (hasAudio()) {
              isAudio = true;
              type = ICallTransportForCall::SocketType_Audio;
            }
          } else if ("video" == description->mType) {
            if (hasVideo()) {
              isVideo = true;
              type = ICallTransportForCall::SocketType_Video;
            }
          }

          if ((!isAudio) && (!isVideo)) {
            ZS_LOG_WARNING(Detail, log("call location media type is not supported") + ", type=" + description->mType)
            continue;
          }

          IICESocketPtr rtpSocket = transport->forCall().getSocket(type, true);
          IICESocketPtr rtcpSocket = transport->forCall().getSocket(type, false);
          if ((!rtpSocket) ||
              (!rtcpSocket)) {
            ZS_LOG_WARNING(Detail, log("failed to object transport's sockets for media type") + ", type=" + description->mType)
            continue;
          }

          if (isAudio) {
            if (!mAudioRTPSocketSession)
              mAudioRTPSocketSession = rtpSocket->createSessionFromRemoteCandidates(mThisWeak.lock(), rtpCandidates, outer->isIncoming() ? IICESocket::ICEControl_Controlled : IICESocket::ICEControl_Controlling);
            if (!mAudioRTCPSocketSession)
              mAudioRTCPSocketSession = rtcpSocket->createSessionFromRemoteCandidates(mThisWeak.lock(), rtcpCandidates, outer->isIncoming() ? IICESocket::ICEControl_Controlled : IICESocket::ICEControl_Controlling);
          } else if (isVideo) {
            if (!mVideoRTPSocketSession)
              mVideoRTPSocketSession = rtpSocket->createSessionFromRemoteCandidates(mThisWeak.lock(), rtpCandidates, outer->isIncoming() ? IICESocket::ICEControl_Controlled : IICESocket::ICEControl_Controlling);
            if (!mVideoRTCPSocketSession)
              mVideoRTCPSocketSession = rtcpSocket->createSessionFromRemoteCandidates(mThisWeak.lock(), rtcpCandidates, outer->isIncoming() ? IICESocket::ICEControl_Controlled : IICESocket::ICEControl_Controlling);
          }
        }

        if (mAudioRTPSocketSession) {
          mAudioRTPSocketSession->setKeepAliveProperties(Seconds(OPENPEER_CALL_RTP_ICE_KEEP_ALIVE_INDICATIONS_SENT_IN_SECONDS), Seconds(OPENPEER_CALL_RTP_ICE_EXPECTING_DATA_WITHIN_IN_SECONDS), Seconds(OPENPEER_CALL_RTP_MAX_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS));
        }
        if (mAudioRTCPSocketSession) {
          mAudioRTCPSocketSession->setKeepAliveProperties(Seconds(OPENPEER_CALL_RTCP_ICE_KEEP_ALIVE_INDICATIONS_SENT_IN_SECONDS), Seconds(OPENPEER_CALL_RTCP_ICE_EXPECTING_DATA_WITHIN_IN_SECONDS));
        }
        if (mVideoRTPSocketSession) {
          mVideoRTPSocketSession->setKeepAliveProperties(Seconds(OPENPEER_CALL_RTP_ICE_KEEP_ALIVE_INDICATIONS_SENT_IN_SECONDS), Seconds(OPENPEER_CALL_RTP_ICE_EXPECTING_DATA_WITHIN_IN_SECONDS), Seconds(OPENPEER_CALL_RTP_MAX_KEEP_ALIVE_REQUEST_TIMEOUT_IN_SECONDS));
        }
        if (mVideoRTCPSocketSession) {
          mVideoRTCPSocketSession->setKeepAliveProperties(Seconds(OPENPEER_CALL_RTCP_ICE_KEEP_ALIVE_INDICATIONS_SENT_IN_SECONDS), Seconds(OPENPEER_CALL_RTCP_ICE_EXPECTING_DATA_WITHIN_IN_SECONDS));
        }

        ZS_LOG_DEBUG(log("init completed") +
                     ", audio RTP session ID=" + Stringize<PUID>(mAudioRTPSocketSession ? mAudioRTPSocketSession->getID() : 0).string() +
                     ", audio RTCP session ID=" + Stringize<PUID>(mAudioRTCPSocketSession ? mAudioRTCPSocketSession->getID() : 0).string() +
                     ", video RTP session ID=" + Stringize<PUID>(mVideoRTPSocketSession ? mVideoRTPSocketSession->getID() : 0).string() +
                     ", video RTCP session ID=" + Stringize<PUID>(mVideoRTCPSocketSession ? mVideoRTCPSocketSession->getID() : 0).string())
      }

      //-----------------------------------------------------------------------
      Call::CallLocation::~CallLocation()
      {
        mThisWeak.reset();
        close();
      }

      //-----------------------------------------------------------------------
      String Call::CallLocation::toDebugString(
                                               CallLocationPtr location,
                                               bool normal,
                                               bool media,
                                               bool includeCallPrefix
                                               )
      {
        if (!location) return includeCallPrefix ? ", calll location=(null)" : "calll location=(null)";
        return location->getDebugValueString(normal, media, includeCallPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call::CallLocation => friend Call
      #pragma mark

      //-----------------------------------------------------------------------
      Call::CallLocationPtr Call::CallLocation::create(
                                                       IMessageQueuePtr queue,
                                                       IMessageQueuePtr mediaQueue,
                                                       CallPtr outer,
                                                       CallTransportPtr transport,
                                                       const char *locationID,
                                                       const DialogPtr &remoteDialog,
                                                       bool hasAudio,
                                                       bool hasVideo
                                                       )
      {
        CallLocationPtr pThis(new CallLocation(queue, mediaQueue, outer, locationID, remoteDialog, hasAudio, hasVideo));
        pThis->mThisWeak = pThis;
        pThis->init(transport);
        return pThis;
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::close()
      {
        ZS_LOG_DEBUG(log("close called on call location"))
        cancel();
      }

      //-----------------------------------------------------------------------
      Call::ICallLocation::CallLocationStates Call::CallLocation::getState() const
      {
        AutoRecursiveLock lock(getLock());
        return mCurrentState;
      }

      //-----------------------------------------------------------------------
      DialogPtr Call::CallLocation::getRemoteDialog() const
      {
        AutoRecursiveLock lock(getLock());
        return mRemoteDialog;
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::updateDialog(const DialogPtr &remoteDialog)
      {
        AutoRecursiveLock lock(getLock());
        mRemoteDialog = remoteDialog;
      }

      //-----------------------------------------------------------------------
      bool Call::CallLocation::sendRTPPacket(
                                             SocketTypes type,
                                             const BYTE *packet,
                                             ULONG packetLengthInBytes
                                             )
      {
        IICESocketSessionPtr session;

        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());

          switch (type) {
            case SocketType_Audio: session = mAudioRTPSocketSession; break;
            case SocketType_Video: session = mVideoRTPSocketSession; break;
          }
        }

        if (!session) {
          ZS_LOG_WARNING(Trace, log("unable to send RTP packet as there is no ICE session object"))
          return false;
        }
        return session->sendPacket(packet, packetLengthInBytes);
      }

      //-----------------------------------------------------------------------
      bool Call::CallLocation::sendRTCPPacket(
                                              SocketTypes type,
                                              const BYTE *packet,
                                              ULONG packetLengthInBytes
                                              )
      {
        IICESocketSessionPtr session;

        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());

          switch (type) {
            case SocketType_Audio: session = mAudioRTCPSocketSession; break;
            case SocketType_Video: session = mVideoRTCPSocketSession; break;
          }
        }

        if (!session) {
          ZS_LOG_WARNING(Trace, log("unable to send RTCP packet as there is no ICE session object"))
          return false;
        }
        return session->sendPacket(packet, packetLengthInBytes);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call::CallLocation => IICESocketSessionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void Call::CallLocation::onICESocketSessionStateChanged(
                                                              IICESocketSessionPtr inSession,
                                                              ICESocketSessionStates state
                                                              )
      {
        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());
          if (isClosed()) {
            ZS_LOG_WARNING(Detail, log("received notification of ICE socket change state but already closed (probably okay)"))
            return;
          }

          bool found = false;
          SocketTypes type = SocketType_Audio;
          bool wasRTP = false;

          IICESocketSessionPtr &session = findSession(inSession, found, &type, &wasRTP);
          if (!found) {
            ZS_LOG_WARNING(Detail, log("ignoring ICE socket session state change on obsolete session") + Stringize<PUID>(inSession->getID()).string())
            return;
          }

          if (IICESocketSession::ICESocketSessionState_Shutdown == state) {
            ZS_LOG_DEBUG(log("ICE socket session is shutdown") + ", type=" + ICallForCallTransport::toString(type) + ", is RTP=" + (wasRTP ? "true" : "false"))
            session.reset();
          }
        }

        ZS_LOG_DEBUG(log("ICE socket session state changed thus invoking step"))
        ICallLocationAsyncProxy::create(getQueue(), mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::handleICESocketSessionReceivedPacket(
                                                                    IICESocketSessionPtr inSession,
                                                                    const zsLib::BYTE *buffer,
                                                                    ULONG bufferLengthInBytes
                                                                    )
      {
        CallPtr outer = mOuter.lock();

        if (!outer) {
          ZS_LOG_WARNING(Trace, log("ignoring ICE socket packet as call object is gone"))
          return;
        }

        bool found = false;
        SocketTypes type = SocketType_Audio;
        bool wasRTP = false;

        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());

          if (isClosed()) {
            ZS_LOG_WARNING(Detail, log("received packet but already closed (probably okay)"))
            return;
          }

          findSession(inSession, found, &type, &wasRTP);
          if (!found) {
            ZS_LOG_WARNING(Trace, log("ignoring ICE socket packet from obsolete session"))
            return;
          }
        }

        if (wasRTP) {
          outer->notifyReceivedRTPPacket(mID, type, buffer, bufferLengthInBytes);
        } else {
          outer->notifyReceivedRTCPPacket(mID, type, buffer, bufferLengthInBytes);
        }
      }

      //-----------------------------------------------------------------------
      bool Call::CallLocation::handleICESocketSessionReceivedSTUNPacket(
                                                                        IICESocketSessionPtr session,
                                                                        STUNPacketPtr stun,
                                                                        const String &localUsernameFrag,
                                                                        const String &remoteUsernameFrag
                                                                        )
      {
        // IGNORED
        return false;
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::onICESocketSessionWriteReady(IICESocketSessionPtr session)
      {
        // IGNORED
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call::CallLocation => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Call::CallLocation::getLock() const
      {
        CallPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      RecursiveLock &Call::CallLocation::getMediaLock() const
      {
        CallPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getMediaLock();
      }

      //-----------------------------------------------------------------------
      String Call::CallLocation::log(const char *message) const
      {
        return String("Call::CallLocation [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String Call::CallLocation::getDebugValueString(
                                                     bool normal,
                                                     bool media,
                                                     bool includeCallPrefix
                                                     ) const
      {
        bool firstTime = !includeCallPrefix;
        String result = Helper::getDebugValue("call location id", Stringize<typeof(mID)>(mID).string(), firstTime) +
                        Helper::getDebugValue("location id", mLocationID, firstTime) +
                        Helper::getDebugValue("audio", mHasAudio ? String("true") : String(), firstTime) +
                        Helper::getDebugValue("video", mHasAudio ? String("true") : String(), firstTime);

        if (normal)
        {
          AutoRecursiveLock lock(getLock());
          result += Dialog::toDebugString(mRemoteDialog);
        }
        if (media)
        {
          AutoRecursiveLock lock(getMediaLock());
          result += Helper::getDebugValue("audio rtp socket session", mAudioRTPSocketSession ? Stringize<PUID>(mAudioRTPSocketSession->getID()).string() : String(), firstTime) +
                    Helper::getDebugValue("audio rtcp socket session", mAudioRTCPSocketSession ? Stringize<PUID>(mAudioRTCPSocketSession->getID()).string() : String(), firstTime) +
                    Helper::getDebugValue("video rtp socket session", mVideoRTPSocketSession ? Stringize<PUID>(mVideoRTPSocketSession->getID()).string() : String(), firstTime) +
                    Helper::getDebugValue("video rtcp socket session", mVideoRTCPSocketSession ? Stringize<PUID>(mVideoRTCPSocketSession->getID()).string() : String(), firstTime);
        }
        return result;
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        // scope: object
        {
          AutoRecursiveLock lock(getLock());
          if (isClosed()) {
            ZS_LOG_DEBUG(log("cancel called but call already shutdown"))
            return;
          }
        }

        setState(CallLocationState_Closed);

        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());

          ZS_LOG_DEBUG(log("shutting down audio/video socket sessions"))

          if (mAudioRTPSocketSession) {
            mAudioRTPSocketSession->close();
            mAudioRTPSocketSession.reset();
          }
          if (mAudioRTCPSocketSession) {
            mAudioRTCPSocketSession->close();
            mAudioRTCPSocketSession.reset();
          }
          if (mVideoRTPSocketSession) {
            mVideoRTPSocketSession->close();
            mVideoRTPSocketSession.reset();
          }
          if (mVideoRTCPSocketSession) {
            mVideoRTCPSocketSession->close();
            mVideoRTCPSocketSession.reset();
          }
        }

        // scope: final object shutdown
        {
          AutoRecursiveLock lock(getLock());
        }

        // scope: final media shutdown
        {
          AutoRecursiveLock lock(getMediaLock());
        }
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::step()
      {
        // scope: object
        {
          AutoRecursiveLock lock(getLock());
          if (isClosed()) {
            ZS_LOG_DEBUG(log("step called by call location is closed thus redirecting to cancel()"))
            goto cancel;
          }
        }

        // scope: media
        {
          AutoRecursiveLock lock(getMediaLock());

          ZS_LOG_DEBUG(log("checking to see if media is setup"))

          if (hasAudio()) {
            if (!mAudioRTPSocketSession) {
              ZS_LOG_WARNING(Detail, log("audio RTP socket session is unexpectedly gone"))
              goto cancel;
            }
            if (!mAudioRTCPSocketSession) {
              ZS_LOG_WARNING(Detail, log("audio RTCP socket session is unexpectedly gone"))
              goto cancel;
            }

            if (IICESocketSession::ICESocketSessionState_Nominated != mAudioRTPSocketSession->getState()) {
              ZS_LOG_DEBUG(log("waiting on audio RTP socket to be nominated...") + ", socket session ID=" + Stringize<PUID>(mAudioRTPSocketSession->getID()).string())
              return;
            }

            if (IICESocketSession::ICESocketSessionState_Nominated != mAudioRTCPSocketSession->getState()) {
              ZS_LOG_DEBUG(log("waiting on audio RTCP socket to be nominated...") + ", socket session ID=" + Stringize<PUID>(mAudioRTCPSocketSession->getID()).string())
              return;
            }
          }

          if (hasVideo()) {
            if (!mVideoRTPSocketSession) {
              ZS_LOG_WARNING(Detail, log("video RTP socket session is unexpectedly gone"))
              goto cancel;
            }
            if (!mVideoRTCPSocketSession) {
              ZS_LOG_WARNING(Detail, log("video RTCP socket session is unexpectedly gone"))
              goto cancel;
            }

            if (mVideoRTPSocketSession) {
              if (IICESocketSession::ICESocketSessionState_Nominated != mVideoRTPSocketSession->getState()) {
                ZS_LOG_DEBUG(log("waiting on video RTP socket to be nominated...") + ", socket session ID=" + Stringize<PUID>(mVideoRTPSocketSession->getID()).string())
                return;
              }
            }

            if (mVideoRTCPSocketSession) {
              if (IICESocketSession::ICESocketSessionState_Nominated != mVideoRTCPSocketSession->getState()) {
                ZS_LOG_DEBUG(log("waiting on video RTCP socket to be nominated...") + ", socket session ID=" + Stringize<PUID>(mVideoRTCPSocketSession->getID()).string())
                return;
              }
            }
          }
        }

        setState(CallLocationState_Ready);

        return;

      cancel:
        cancel();
      }

      //-----------------------------------------------------------------------
      void Call::CallLocation::setState(CallLocationStates state)
      {
        // scope: object
        {
          AutoRecursiveLock lock(getLock());
          if (state <= mCurrentState) return;

          ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

          mCurrentState = state;
        }

        CallPtr outer = mOuter.lock();
        if (outer) {
          CallLocationPtr pThis = mThisWeak.lock();
          if (pThis) {
            outer->notifyStateChanged(pThis, state);
          }
        }
      }

      //-----------------------------------------------------------------------
      IICESocketSessionPtr &Call::CallLocation::findSession(
                                                            IICESocketSessionPtr session,
                                                            bool &outFound,
                                                            SocketTypes *outType,
                                                            bool *outIsRTP
                                                            )
      {
        static IICESocketSessionPtr bogus;

        outFound = true;
        if (outType) *outType = SocketType_Audio;
        if (outIsRTP) *outIsRTP = true;

        if (!session) return bogus;

        if (hasAudio()) {
          if (session == mAudioRTPSocketSession) {
            return mAudioRTPSocketSession;
          }
          if (session == mAudioRTCPSocketSession) {
            if (outIsRTP) *outIsRTP = false;
            return mAudioRTCPSocketSession;
          }
        }

        if (outType) *outType = SocketType_Video;
        if (hasVideo()) {
          if (session == mVideoRTPSocketSession) {
            return mVideoRTPSocketSession;
          }
          if (session == mVideoRTCPSocketSession) {
            if (outIsRTP) *outIsRTP = false;
            return mVideoRTCPSocketSession;
          }
        }

        ZS_LOG_WARNING(Trace, log("did not find socket session thus returning bogus session") + ", socket session ID=" + Stringize<PUID>(session->getID()).string())

        outFound = false;
        if (outType) *outType = SocketType_Audio;
        if (outIsRTP) *outIsRTP = false;
        return bogus;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICall
    #pragma mark

    //-------------------------------------------------------------------------
    String ICall::toDebugString(ICallPtr call, bool includeCommaPrefix)
    {
      return internal::Call::toDebugString(call, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    const char *ICall::toString(CallStates state)
    {
      return internal::IConversationThreadParser::Dialog::toString(internal::convert(state));
    }

    //-------------------------------------------------------------------------
    const char *ICall::toString(CallClosedReasons reason)
    {
      return internal::IConversationThreadParser::Dialog::toString(internal::convert(reason));
    }

    //-------------------------------------------------------------------------
    ICallPtr ICall::placeCall(
                              IConversationThreadPtr conversationThread,
                              IContactPtr toContact,
                              bool includeAudio,
                              bool includeVideo
                              )
    {
      return internal::ICallFactory::singleton().placeCall(conversationThread, toContact, includeAudio, includeVideo);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }
}
