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

#include <hookflash/core/internal/types.h>
//#include <hookflash/core/internal/core_IConversationThreadParser.h>
#include <hookflash/core/internal/core_MediaEngine.h>
//
//#include <hookflash/services/IICESocket.h>
//
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

namespace hookflash
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

      interaction ICallTransport
      {
        enum CallTransportStates
        {
          CallTransportState_Pending,
          CallTransportState_Ready,
          CallTransportState_ShuttingDown,
          CallTransportState_Shutdown,
        };

        static const char *toString(CallTransportStates state);

        static String toDebugString(ICallTransportPtr transport, bool includeCommaPrefix = true);

        virtual PUID getID() const = 0;
        virtual CallTransportStates getState() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportDelegate
      #pragma mark

      interaction ICallTransportDelegate
      {
        typedef ICallTransport::CallTransportStates CallTransportStates;

        virtual void onCallTransportStateChanged(
                                                 ICallTransportPtr transport,
                                                 CallTransportStates state
                                                 ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportForAccount
      #pragma mark

      interaction ICallTransportForAccount
      {
        typedef ICallTransport::CallTransportStates CallTransportStates;

        ICallTransportForAccount &forAccount() {return *this;}
        const ICallTransportForAccount &forAccount() const {return *this;}

        static CallTransportPtr create(
                                       ICallTransportDelegatePtr delegate,
                                       const char *turnServer,
                                       const char *turnServerUsername,
                                       const char *turnServerPassword,
                                       const char *stunServer
                                       );

        virtual void shutdown() = 0;
        virtual CallTransportStates getState() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportForCall
      #pragma mark

      interaction ICallTransportForCall
      {
        enum SocketTypes
        {
          SocketType_Audio,
          SocketType_Video,
        };

        static const char *toString(SocketTypes type);

        ICallTransportForCall &forCall() {return *this;}
        const ICallTransportForCall &forCall() const {return *this;}

        virtual RecursiveLock &getLock() const = 0;

        virtual IMessageQueuePtr getMediaQueue() const = 0;

        virtual void notifyCallCreation(PUID idCall) = 0;
        virtual void notifyCallDestruction(PUID idCall) = 0;

        virtual void focus(
                           CallPtr call,
                           PUID locationID
                           ) = 0;
        virtual void loseFocus(PUID callID) = 0;

        virtual IICESocketPtr getSocket(
                                        SocketTypes type,
                                        bool forRTP
                                        ) const = 0;

        virtual void notifyReceivedRTPPacket(
                                             PUID callID,
                                             PUID locationID,
                                             SocketTypes type,
                                             const BYTE *buffer,
                                             ULONG bufferLengthInBytes
                                             ) = 0;

        virtual void notifyReceivedRTCPPacket(
                                              PUID callID,
                                              PUID locationID,
                                              SocketTypes type,
                                              const BYTE *buffer,
                                              ULONG bufferLengthInBytes
                                              ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICallTransportAsync
      #pragma mark

      interaction ICallTransportAsync
      {
        virtual void onStep() = 0;

        virtual void onStart() = 0;
        virtual void onStop() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark CallTransport

      class CallTransport  : public MessageQueueAssociator,
                             public ICallTransport,
                             public ICallTransportForAccount,
                             public ICallTransportForCall,
                             public ICallTransportAsync,
                             public ITimerDelegate
      {
      public:
        friend interaction ICallTransport;
        friend interaction ICallTransportForAccount;
        friend interaction ICallTransportForCall;

        typedef ICallTransport::CallTransportStates CallTransportStates;

        class TransportSocket;
        typedef boost::shared_ptr<TransportSocket> TransportSocketPtr;
        typedef boost::weak_ptr<TransportSocket> TransportSocketWeakPtr;
        friend class TransportSocket;

        typedef std::list<TransportSocketPtr> TransportSocketList;

      protected:
        CallTransport(
                      IMessageQueuePtr queue,
                      ICallTransportDelegatePtr delegate,
                      const char *turnServer,
                      const char *turnServerUsername,
                      const char *turnServerPassword,
                      const char *stunServer
                      );

        void init();

      public:
        ~CallTransport();

        static CallTransportPtr convert(ICallTransportPtr transport);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark CallTransport => ICallTransport
        #pragma mark

        virtual PUID getID() const {return mID;}
        // (duplicate) virtual CallTransportStates getState() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark CallTransport => ICallTransportForAccount
        #pragma mark

        static CallTransportPtr create(
                                       ICallTransportDelegatePtr delegate,
                                       const char *turnServer,
                                       const char *turnServerUsername,
                                       const char *turnServerPassword,
                                       const char *stunServer
                                       );

        virtual void shutdown();
        virtual CallTransportStates getState() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark CallTransport => ICallTransportForCall
        #pragma mark

        virtual RecursiveLock &getLock() const;

        virtual IMessageQueuePtr getMediaQueue() const;

        virtual void notifyCallCreation(PUID idCall);
        virtual void notifyCallDestruction(PUID idCall);

        virtual void focus(
                           CallPtr call,
                           PUID locationID
                           );
        virtual void loseFocus(PUID callID);

        virtual IICESocketPtr getSocket(
                                        SocketTypes type,
                                        bool forRTP
                                        ) const;

        virtual void notifyReceivedRTPPacket(
                                             PUID callID,
                                             PUID locationID,
                                             SocketTypes type,
                                             const BYTE *buffer,
                                             ULONG bufferLengthInBytes
                                             );

        virtual void notifyReceivedRTCPPacket(
                                              PUID callID,
                                              PUID locationID,
                                              SocketTypes type,
                                              const BYTE *buffer,
                                              ULONG bufferLengthInBytes
                                              );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark CallTransport => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark CallTransport => ICallTransportAsync
        #pragma mark

        virtual void onStep() {step();}
        virtual void onStart() {start();}
        virtual void onStop() {stop();}

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark CallTransport => friend TransportSocket
        #pragma mark

        int sendRTPPacket(PUID socketID, const void *data, int len);
        int sendRTCPPacket(PUID socketID, const void *data, int len);

      protected:
        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        bool isPending() const {return CallTransportState_Pending == mCurrentState;}
        bool isReady() const {return CallTransportState_Ready == mCurrentState;}
        bool isShuttingDown() const {return CallTransportState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return CallTransportState_ShuttingDown == mCurrentState;}

        void start();
        void stop();

        void cancel();
        void step();
        void setState(CallTransportStates state);

        void fixSockets();
        bool cleanObsoleteSockets();

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark CallTransport::TransportSocket

        class TransportSocket : public MessageQueueAssociator,
                                public IICESocketDelegate,
                                public webrtc::Transport
        {
        public:
          friend class CallTransport;

        protected:
          TransportSocket(
                          IMessageQueuePtr queue,
                          CallTransportPtr outer
                          );

          void init(
                    const char *turnServer,
                    const char *turnServerUsername,
                    const char *turnServerPassword,
                    const char *stunServer
                    );

        public:
          ~TransportSocket();

          static String toDebugString(TransportSocketPtr socket, bool includeCommaPrefix = true);

          //-------------------------------------------------------------------
          #pragma mark CallTransport::TransportSocket => friend CallTransport

          static TransportSocketPtr create(
                                           IMessageQueuePtr queue,
                                           CallTransportPtr outer,
                                           const char *turnServer,
                                           const char *turnServerUsername,
                                           const char *turnServerPassword,
                                           const char *stunServer
                                           );

          PUID getID() const {return mID;}

          IICESocketPtr getRTPSocket() const {return mRTPSocket;}
          IICESocketPtr getRTCPSocket() const {return mRTCPSocket;}

          void shutdown();
          bool isReady() const;
          bool isShutdown() const;

          //-------------------------------------------------------------------
          #pragma mark CallTransport::TransportSocket => IICESocketDelegate

          virtual void onICESocketStateChanged(
                                               IICESocketPtr socket,
                                               ICESocketStates state
                                               );

          //-------------------------------------------------------------------
          #pragma mark CallTransport::TransportSocket => webrtc::Transport

          virtual int SendPacket(int channel, const void *data, int len);
          virtual int SendRTCPPacket(int channel, const void *data, int len);

        protected:
          //-------------------------------------------------------------------
          #pragma mark CallTransport::TransportSocket => (internal)
          String log(const char *message) const;
          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark CallTransport::TransportSocket => (data)
          PUID mID;
          TransportSocketWeakPtr mThisWeak;
          CallTransportWeakPtr mOuter;

          IICESocketPtr mRTPSocket;
          IICESocketPtr mRTCPSocket;
        };

      protected:
        //-------------------------------------------------------------------
        #pragma mark CallTransport => (data)
        PUID mID;
        mutable RecursiveLock mLock;
        CallTransportWeakPtr mThisWeak;
        CallTransportPtr mGracefulShutdownReference;

        ICallTransportDelegatePtr mDelegate;

        String mTURNServer;
        String mTURNServerUsername;
        String mTURNServerPassword;
        String mSTUNServer;

        CallTransportStates mCurrentState;

        ULONG mTotalCalls;
        TimerPtr mSocketCleanupTimer;

        bool mStarted;
        CallWeakPtr mFocus;
        PUID mFocusCallID;
        PUID mFocusLocationID;
        bool mHasAudio;
        bool mHasVideo;
        ULONG mBlockUntilStartStopCompleted;

        TransportSocketPtr mAudioSocket;
        TransportSocketPtr mVideoSocket;

        PUID mAudioSocketID;
        PUID mVideoSocketID;

        TransportSocketList mObsoleteSockets;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::ICallTransportDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::internal::ICallTransportPtr, ICallTransportPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::core::internal::ICallTransport::CallTransportStates, CallTransportStates)
ZS_DECLARE_PROXY_METHOD_2(onCallTransportStateChanged, ICallTransportPtr, CallTransportStates)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::ICallTransportAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_METHOD_0(onStart)
ZS_DECLARE_PROXY_METHOD_0(onStop)
ZS_DECLARE_PROXY_END()
