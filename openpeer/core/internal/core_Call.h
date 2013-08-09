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

#pragma once

#include <openpeer/core/internal/types.h>
#include <openpeer/core/internal/core_IConversationThreadParser.h>
#include <openpeer/core/internal/core_ConversationThread.h>
#include <openpeer/core/ICall.h>

#include <openpeer/services/IICESocket.h>
#include <openpeer/services/IICESocketSession.h>
#include <openpeer/services/IWakeDelegate.h>

#include <zsLib/Timer.h>

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
      #pragma mark ICallForConversationThread
      #pragma mark

      interaction ICallForConversationThread
      {
        typedef IConversationThreadParser::DialogPtr DialogPtr;

        ICallForConversationThread &forConversationThread() {return *this;}
        const ICallForConversationThread &forConversationThread() const {return *this;}

        static CallPtr createForIncomingCall(
                                             ConversationThreadPtr inConversationThread,
                                             ContactPtr callerContact,
                                             const DialogPtr &remoteDialog
                                             );

        virtual String getCallID() const = 0;

        virtual ContactPtr getCaller(bool ignored = true) const = 0;
        virtual ContactPtr getCallee(bool ignored = true) const = 0;

        virtual DialogPtr getDialog() const = 0;

        virtual void notifyConversationThreadUpdated() = 0;

      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallForCallTransport
      #pragma mark

      interaction ICallForCallTransport
      {
        enum SocketTypes
        {
          SocketType_Audio,
          SocketType_Video,
        };

        static const char *toString(SocketTypes type);

        ICallForCallTransport &forCallTransport() {return *this;}
        const ICallForCallTransport &forCallTransport() const {return *this;}

        virtual PUID getID() const = 0;

        virtual bool hasAudio() const = 0;
        virtual bool hasVideo() const = 0;
        
        virtual MediaSessionList getReceiveMediaSessions() = 0;
        virtual MediaSessionList getSendMediaSessions() = 0;
        virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true) = 0;
        virtual void removeMediaSession(IMediaSessionPtr session) = 0;

        virtual void notifyLostFocus() = 0;

        virtual bool sendRTPPacket(
                                   PUID toLocationID,
                                   SocketTypes type,
                                   const BYTE *packet,
                                   ULONG packetLengthInBytes
                                   ) = 0;

        virtual bool sendRTCPPacket(
                                    PUID toLocationID,
                                    SocketTypes type,
                                    const BYTE *packet,
                                    ULONG packetLengthInBytes
                                    ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallAsync
      #pragma mark

      interaction ICallAsync
      {
        virtual void onSetFocus(bool on) = 0;
        virtual void onHangup() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Call
      #pragma mark

      class Call  : public Noop,
                    // public zsLib::MessageQueueAssociator,  // we do NOT want to inherit from an queue associator object will use multiple queues
                    public ICall,
                    public ICallForConversationThread,
                    public ICallForCallTransport,
                    public IWakeDelegate,
                    public ICallAsync,
                    public IICESocketDelegate,
                    public ITimerDelegate
      {
      public:
        friend interaction ICallFactory;

        struct Exceptions
        {
          ZS_DECLARE_CUSTOM_EXCEPTION(StepFailure)
          ZS_DECLARE_CUSTOM_EXCEPTION(IllegalState)
          ZS_DECLARE_CUSTOM_EXCEPTION(CallClosed)
        };

        typedef IConversationThreadParser::DialogPtr DialogPtr;
        typedef IConversationThreadForCall::LocationDialogMap LocationDialogMap;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call::ICallLocation
        #pragma mark

        interaction ICallLocation
        {
          enum CallLocationStates
          {
            CallLocationState_Pending = 0,
            CallLocationState_Ready = 1,
            CallLocationState_Closed = 2,
          };

          static const char *toString(CallLocationStates state);
        };
        typedef ICallLocation::CallLocationStates CallLocationStates;

        class CallLocation;
        typedef boost::shared_ptr<CallLocation> CallLocationPtr;
        typedef boost::weak_ptr<CallLocation> CallLocationWeakPtr;
        typedef std::list<CallLocationPtr> CallLocationList;
        friend class CallLocation;

      protected:
        Call(
             AccountPtr account,
             ConversationThreadPtr conversationThread,
             ICallDelegatePtr delegate,
             bool hasAudio,
             bool hasVideo,
             const char *callID
             );
        Call(Noop) : Noop(true) {}

        void init();

      public:
        ~Call();

        static CallPtr convert(ICallPtr call);

        static String toDebugString(ICallPtr call, bool includeCommaPrefix = true);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => ICall
        #pragma mark

        static CallPtr placeCall(
                                 IConversationThreadPtr conversationThread,
                                 IContactPtr toContact,
                                 bool includeAudio,
                                 bool includeVideo
                                 );

        virtual String getCallID() const;

        virtual IConversationThreadPtr getConversationThread() const;

        virtual IContactPtr getCaller() const;
        virtual IContactPtr getCallee() const;

        virtual bool hasAudio() const;
        virtual bool hasVideo() const;
        
        virtual MediaSessionList getReceiveMediaSessions();
        virtual MediaSessionList getSendMediaSessions();
        virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true);
        virtual void removeMediaSession(IMediaSessionPtr session);

        virtual CallStates getState() const;
        virtual CallClosedReasons getClosedReason() const;

        virtual Time getcreationTime() const;
        virtual Time getRingTime() const;
        virtual Time getAnswerTime() const;
        virtual Time getClosedTime() const;

        virtual void ring();
        virtual void answer();
        virtual void hold(bool hold);
        virtual void hangup(CallClosedReasons reason = CallClosedReason_User);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => ICallForCallTransport
        #pragma mark

        virtual PUID getID() const {return mID;}

        // (duplicate) virtual bool hasAudio() const;
        // (duplicate) virtual bool hasVideo() const;
        
        // (duplicate) virtual MediaSessionList getReceiveMediaSessions();
        // (duplicate) virtual MediaSessionList getSendMediaSessions();
        // (duplicate) virtual void addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams = true);
        // (duplicate) virtual void removeMediaSession(IMediaSessionPtr session);

        virtual void notifyLostFocus();

        virtual bool sendRTPPacket(
                                   PUID toLocationID,
                                   SocketTypes type,
                                   const BYTE *packet,
                                   ULONG packetLengthInBytes
                                   );

        virtual bool sendRTCPPacket(
                                    PUID toLocationID,
                                    SocketTypes type,
                                    const BYTE *packet,
                                    ULONG packetLengthInBytes
                                    );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => ICallForConversationThread
        #pragma mark

        static CallPtr createForIncomingCall(
                                             ConversationThreadPtr inConversationThread,
                                             ContactPtr callerContact,
                                             const DialogPtr &remoteDialog
                                             );

        // (duplicate) virtual String getCallID() const;

        virtual DialogPtr getDialog() const;

        virtual ContactPtr getCaller(bool) const;
        virtual ContactPtr getCallee(bool) const;

        virtual void notifyConversationThreadUpdated();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => IWakeDelegate
        #pragma mark

        virtual void onWake() {step();}

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => ICallAsync
        #pragma mark

        virtual void onSetFocus(bool on);
        virtual void onHangup() {cancel();}

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => IICESocketDelegate
        #pragma mark

        virtual void onICESocketStateChanged(
                                             IICESocketPtr socket,
                                             ICESocketStates state
                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => ITimerDelegate
        #pragma mark
        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => friend Call::CallLocation
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        // (duplicate) RecursiveLock &getLock() const;
        // (duplicate) RecursiveLock &getMediaLock() const;

        virtual void notifyReceivedRTPPacket(
                                             PUID locationID,
                                             SocketTypes type,
                                             const BYTE *buffer,
                                             ULONG bufferLengthInBytes
                                             );

        virtual void notifyReceivedRTCPPacket(
                                              PUID locationID,
                                              SocketTypes type,
                                              const BYTE *buffer,
                                              ULONG bufferLengthInBytes
                                              );

        void notifyStateChanged(
                                CallLocationPtr location,
                                CallLocationStates state
                                );

        bool isIncoming() const {return mIncomingCall;}

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        RecursiveLock &getMediaLock() const;
        RecursiveLock &getStepLock() const;

        IMessageQueuePtr getQueue() const {return mQueue;}
        IMessageQueuePtr getMediaQueue() const {return mMediaQueue;}

      private:
        String log(const char *message) const;
        virtual String getDebugValueString(
                                           bool callData,
                                           bool mediaData,
                                           bool includeCommaPrefix = true
                                           ) const;

        bool isShuttingdown() const;
        bool isShutdown() const;

        void cancel();

        void checkState(
                        CallStates state,
                        bool isLocal,
                        bool *callClosedNoThrow = NULL
                        ) const throw (
                                       Exceptions::IllegalState,
                                       Exceptions::CallClosed
                                       );

        void checkLegalWhenNotPicked() const throw (Exceptions::IllegalState);
        void checkLegalWhenPicked(
                                  CallStates state,
                                  bool isLocal
                                  ) const throw (Exceptions::IllegalState);

        bool isLockedToAnotherLocation(const DialogPtr &remoteDialog) const;

        bool stepIsMediaReady(
                              bool needCandidates,
                              CandidateList &outAudioRTPCandidates,
                              CandidateList &outAudioRTCPCandidates,
                              CandidateList &outVideoRTPCandidates,
                              CandidateList &outVideoRTCPCandidates
                              ) throw (Exceptions::StepFailure);

        bool stepPrepareCallFirstTime(
                                      CallLocationPtr &picked,
                                      const CandidateList &audioRTPCandidates,
                                      const CandidateList &audioRTCPCandidates,
                                      const CandidateList &videoRTPCandidates,
                                      const CandidateList &videoRTCPCandidates
                                      ) throw (Exceptions::StepFailure);

        bool stepPrepareCallLocations(
                                      CallLocationPtr &picked,
                                      const LocationDialogMap locationDialogMap,
                                      CallLocationList &outLocationsToClose
                                      ) throw (Exceptions::CallClosed);

        bool stepVerifyCallState(CallLocationPtr &picked) throw (
                                                                 Exceptions::IllegalState,
                                                                 Exceptions::CallClosed
                                                                 );

        bool stepTryToPickALocation(
                                    AccountPtr &account,
                                    CallLocationPtr &ioEarly,
                                    CallLocationPtr &ioPicked,
                                    CallLocationList &outLocationsToClose
                                    );

        bool stepHandlePickedLocation(
                                      bool &ioMediaHolding,
                                      CallLocationPtr &picked
                                      ) throw (Exceptions::IllegalState);

        bool stepFixCallInProgressStates(
                                         AccountPtr &account,
                                         bool mediaHolding,
                                         CallLocationPtr &early,
                                         CallLocationPtr &picked
                                         );

        bool stepCloseLocations(CallLocationList &locationsToClose);

        void step();
        void setCurrentState(
                             CallStates state,
                             bool forceOverride = false
                             );

        void setClosedReason(CallClosedReasons reason);

        IICESocketSubscriptionPtr &findSubscription(
                                                    IICESocketPtr socket,
                                                    bool &outFound,
                                                    SocketTypes *outType = NULL,
                                                    bool *outIsRTP = NULL
                                                    );
        bool placeCallWithConversationThread();

      public:

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call::CallLocation
        #pragma mark

        class CallLocation : public ICallLocation,
                             public services::IICESocketSessionDelegate,
                             public IWakeDelegate
        {
        public:

        protected:
          CallLocation(
                       IMessageQueuePtr queue,
                       IMessageQueuePtr mediaQueue,
                       CallPtr outer,
                       const char *locationID,
                       const DialogPtr &remoteDialog,
                       bool hasAudio,
                       bool hasVideo
                       );

          void init(CallTransportPtr tranasport);

        public:
          ~CallLocation();

          static String toDebugString(
                                      CallLocationPtr location,
                                      bool normal,
                                      bool media,
                                      bool includeCallPrefix = true
                                      );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Call::CallLocation => friend Call
          #pragma mark

          static CallLocationPtr create(
                                        IMessageQueuePtr queue,
                                        IMessageQueuePtr mediaQueue,
                                        CallPtr outer,
                                        CallTransportPtr transport,
                                        const char *locationID,
                                        const DialogPtr &remoteDialog,
                                        bool hasAudio,
                                        bool hasVideo
                                        );

          PUID getID() const {return mID;}
          const String &getLocationID() const {return mLocationID;}

          void close();

          CallLocationStates getState() const;
          DialogPtr getRemoteDialog() const;

          void updateDialog(const DialogPtr &remoteDialog);

          bool sendRTPPacket(
                             SocketTypes type,
                             const BYTE *packet,
                             ULONG packetLengthInBytes
                             );

          bool sendRTCPPacket(
                              SocketTypes type,
                              const BYTE *packet,
                              ULONG packetLengthInBytes
                              );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Call::CallLocation => IICESocketSessionDelegate
          #pragma mark

          virtual void onICESocketSessionStateChanged(
                                                      IICESocketSessionPtr session,
                                                      ICESocketSessionStates state
                                                      );

          virtual void handleICESocketSessionReceivedPacket(
                                                            IICESocketSessionPtr session,
                                                            const BYTE *buffer,
                                                            ULONG bufferLengthInBytes
                                                            );

          virtual bool handleICESocketSessionReceivedSTUNPacket(
                                                                IICESocketSessionPtr session,
                                                                STUNPacketPtr stun,
                                                                const String &localUsernameFrag,
                                                                const String &remoteUsernameFrag
                                                                );

          virtual void onICESocketSessionWriteReady(IICESocketSessionPtr session);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Call::CallLocation => IWakeDelegate
          #pragma mark

          virtual void onWake() {step();}

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Call::CallLocation => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          RecursiveLock &getMediaLock() const;

          String log(const char *message) const;
          virtual String getDebugValueString(
                                             bool normal,
                                             bool media,
                                             bool includeCallPrefix = true
                                             ) const;

          bool isPending() const  {return CallLocationState_Pending == mCurrentState;}
          bool isReady() const    {return CallLocationState_Ready == mCurrentState;}
          bool isClosed() const   {return CallLocationState_Closed == mCurrentState;}

          bool hasAudio() const   {return mHasAudio;}
          bool hasVideo() const   {return mHasVideo;}

          IMessageQueuePtr getQueue() const {return mQueue;}
          IMessageQueuePtr getMediaQueue() const {return mMediaQueue;}

          void cancel();
          void step();
          void setState(CallLocationStates state);

          IICESocketSessionPtr &findSession(
                                            IICESocketSessionPtr session,
                                            bool &outFound,
                                            SocketTypes *outType = NULL,
                                            bool *outIsRTP = NULL
                                            );

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Call::CallLocation => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          CallLocationWeakPtr mThisWeak;
          CallLocationStates mCurrentState;

          CallWeakPtr mOuter;
          IMessageQueuePtr mQueue;
          IMessageQueuePtr mMediaQueue;

          String mLocationID;
          bool mHasAudio;
          bool mHasVideo;

          //-------------------------------------------------------------------
          // variables protected with object lock
          DialogPtr mRemoteDialog;

          //-------------------------------------------------------------------
          // variables protected with media lock
          IICESocketSessionPtr mAudioRTPSocketSession;
          IICESocketSessionPtr mAudioRTCPSocketSession;

          IICESocketSessionPtr mVideoRTPSocketSession;
          IICESocketSessionPtr mVideoRTCPSocketSession;
        };

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Call => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        CallWeakPtr mThisWeak;
        CallPtr mGracefulShutdownReference;

        mutable RecursiveLock mStepLock;

        IMessageQueuePtr mQueue;
        IMessageQueuePtr mMediaQueue;

        ICallDelegatePtr mDelegate;

        String mCallID;

        bool mHasAudio;
        bool mHasVideo;

        bool mIncomingCall;
        bool mIncomingNotifiedThreadOfPreparing;

        AccountWeakPtr mAccount;
        ConversationThreadWeakPtr mConversationThread;
        CallTransportPtr mTransport;

        ContactPtr mCaller;
        ContactPtr mCallee;

        //---------------------------------------------------------------------
        // variables protected with object lock

        CallStates mCurrentState;
        CallClosedReasons mClosedReason;

        typedef String LocationID;
        typedef std::map<LocationID, CallLocationPtr> CallLocationMap;
        CallLocationMap mCallLocations;

        TimerPtr mPeerAliveTimer;

        TimerPtr mCleanupTimer;
        bool mPlaceCall;

        DialogPtr mDialog;

        bool mRingCalled;
        bool mAnswerCalled;
        bool mLocalOnHold;

        Time mCreationTime;
        Time mRingTime;
        Time mAnswerTime;
        Time mClosedTime;
        Time mFirstClosedRemoteCallTime;

        TimerPtr mFirstClosedRemoteCallTimer;

        //---------------------------------------------------------------------
        // variables protected with media lock

        IICESocketSubscriptionPtr mAudioRTPSocketSubscription;
        IICESocketSubscriptionPtr mAudioRTCPSocketSubscription;

        IICESocketSubscriptionPtr mVideoRTPSocketSubscription;
        IICESocketSubscriptionPtr mVideoRTCPSocketSubscription;

        bool mMediaHolding;
        CallLocationPtr mPickedLocation;
        CallLocationPtr mEarlyLocation;

        bool mNotifiedCallTransportDestroyed;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallFactory
      #pragma mark

      interaction ICallFactory
      {
        static ICallFactory &singleton();

        virtual CallPtr placeCall(
                                  IConversationThreadPtr conversationThread,
                                  IContactPtr toContact,
                                  bool includeAudio,
                                  bool includeVideo
                                  );

        virtual CallPtr createForIncomingCall(
                                              ConversationThreadPtr inConversationThread,
                                              ContactPtr callerContact,
                                              const DialogPtr &remoteDialog
                                              );
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::internal::ICallAsync)
ZS_DECLARE_PROXY_METHOD_1(onSetFocus, bool)
ZS_DECLARE_PROXY_METHOD_0(onHangup)
ZS_DECLARE_PROXY_END()
