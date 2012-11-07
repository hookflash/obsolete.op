/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/internal/hookflashTypes.h>
#include <hookflash/internal/hookflash_ConversationThread.h>
#include <hookflash/internal/hookflash_ConversationThreadDocumentFetcher.h>
#include <hookflash/internal/hookflash_IConversationThreadParser.h>

#include <hookflash/stack/IPeerSubscription.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>
#include <zsLib/Timer.h>

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadSlaveForConversationThread
    #pragma mark

    interaction IConversationThreadSlaveForConversationThread : public IConversationThreadHostSlaveBase
    {
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IHelper::SplitMap SplitMap;

      static IConversationThreadSlaveForConversationThreadPtr create(
                                                                     IConversationThreadForSlavePtr baseThread,
                                                                     IPeerLocationPtr peerLocation,
                                                                     IPublicationMetaDataPtr metaData,
                                                                     const SplitMap &split
                                                                     );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadSlaveAsync
    #pragma mark

    interaction IConversationThreadSlaveAsync
    {
      virtual void onStep() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadSlave
    #pragma mark

    class ConversationThreadSlave  : public zsLib::MessageQueueAssociator,
                                     public IConversationThreadSlaveForConversationThread,
                                     public IConversationThreadDocumentFetcherDelegate,
                                     public IConversationThreadSlaveAsync,
                                     public zsLib::ITimerDelegate,
                                     public stack::IPeerSubscriptionDelegate
    {
    public:
      enum ConversationThreadSlaveStates
      {
        ConversationThreadSlaveState_Pending,
        ConversationThreadSlaveState_Ready,
        ConversationThreadSlaveState_ShuttingDown,
        ConversationThreadSlaveState_Shutdown,
      };

      static const char *toString(ConversationThreadSlaveStates state);

      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;
      typedef zsLib::TimerPtr TimerPtr;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IHelper::SplitMap SplitMap;
      typedef stack::IPeerSubscriptionPtr IPeerSubscriptionPtr;
      typedef stack::IPeerSubscriptionMessagePtr IPeerSubscriptionMessagePtr;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;
      typedef IConversationThreadParser::Message Message;
      typedef IConversationThreadParser::MessagePtr MessagePtr;
      typedef IConversationThreadParser::Thread Thread;
      typedef IConversationThreadParser::ThreadPtr ThreadPtr;

    protected:
      ConversationThreadSlave(
                              IAccountForConversationThreadPtr account,
                              IPeerLocationPtr peerLocation,
                              IConversationThreadForSlavePtr baseThread,
                              const char *threadID
                              );

      void init();

    public:
      ~ConversationThreadSlave();

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadHostSlaveBase
      #pragma mark

      virtual const String &getThreadID() const;

      virtual bool isHost() const {return false;}
      virtual bool isSlave() const {return true;}

      virtual void shutdown();
      // (duplicate) virtual bool isShutdown() const;

      virtual IConversationThreadHostForConversationThreadPtr toHost() const {return IConversationThreadHostForConversationThreadPtr();}
      virtual IConversationThreadSlaveForConversationThreadPtr toSlave() const {return mThisWeak.lock();}

      virtual bool isHostThreadOpen() const;

      virtual void notifyPublicationUpdated(
                                            IPeerLocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData,
                                            const SplitMap &split
                                            );

      virtual void notifyPublicationGone(
                                         IPeerLocationPtr peerLocation,
                                         IPublicationMetaDataPtr metaData,
                                         const SplitMap &split
                                         );

      virtual void notifyPeerDisconnected(IPeerLocationPtr peerLocation);

      virtual bool sendMessages(const MessageList &messages);

      virtual Time getHostCreationTime() const;

      virtual bool safeToChangeContacts() const;

      virtual void getContacts(ContactMap &outContacts) const;
      virtual bool inConversation(IContactForConversationThreadPtr contact) const;
      virtual void addContacts(const ContactInfoList &contacts);
      virtual void removeContacts(const ContactList &contacts);
      
      virtual ContactStates getContactState(IContactForConversationThreadPtr contact) const;

      virtual bool placeCalls(const PendingCallMap &pendingCalls);
      virtual void notifyCallStateChanged(ICallForConversationThreadPtr call);
      virtual void notifyCallCleanup(ICallForConversationThreadPtr call);

      virtual void gatherDialogReplies(
                                       const char *callID,
                                       LocationDialogMap &outDialogs
                                       ) const;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadSlaveForConversationThread
      #pragma mark

      static ConversationThreadSlavePtr create(
                                               IConversationThreadForSlavePtr baseThread,
                                               IPeerLocationPtr peerLocation,
                                               IPublicationMetaDataPtr metaData,
                                               const SplitMap &split
                                               );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadDocumentFetcherDelegate
      #pragma mark

      virtual void onConversationThreadDocumentFetcherPublicationUpdated(
                                                                         IConversationThreadDocumentFetcherPtr fetcher,
                                                                         IPeerLocationPtr peerLocation,
                                                                         IPublicationPtr publication
                                                                         );

      virtual void onConversationThreadDocumentFetcherPublicationGone(
                                                                      IConversationThreadDocumentFetcherPtr fetcher,
                                                                      IPeerLocationPtr peerLocation,
                                                                      IPublicationMetaDataPtr metaData
                                                                      );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadSlaveAsync
      #pragma mark

      virtual void onStep() {step();}

      //---------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => ITimerDelegate
      #pragma mark

      virtual void onTimer(TimerPtr timer);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IPeerSubscriptionDelegate
      #pragma mark

      virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription);

      virtual void onPeerSubscriptionFindStateChanged(
                                                      IPeerSubscriptionPtr subscription,
                                                      PeerSubscriptionFindStates state
                                                      );

      virtual void onPeerSubscriptionLocationsChanged(IPeerSubscriptionPtr subscription);

      virtual void onPeerSubscriptionMessage(
                                             IPeerSubscriptionPtr subscription,
                                             IPeerSubscriptionMessagePtr incomingMessage
                                             );

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => (internal)
      #pragma mark

      bool isPending() const {return ConversationThreadSlaveState_Pending == mCurrentState;}
      bool isReady() const {return ConversationThreadSlaveState_Ready == mCurrentState;}
      bool isShuttingDown() const {return ConversationThreadSlaveState_ShuttingDown == mCurrentState;}
      virtual bool isShutdown() const {AutoRecursiveLock lock(getLock()); return ConversationThreadSlaveState_Shutdown == mCurrentState;}

      String log(const char *message) const;

      RecursiveLock &getLock() const;

      void cancel();
      void step();

      void setState(ConversationThreadSlaveStates state);

      IContactForConversationThreadPtr getHostContact() const;
      void publish(
                   bool publishSlavePublication,
                   bool publishSlavePermissionPublication
                   ) const;

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => (data)
      #pragma mark

      mutable RecursiveLock mBogusLock;
      PUID mID;
      ConversationThreadSlaveWeakPtr mThisWeak;
      ConversationThreadSlavePtr mGracefulShutdownReference;
      ConversationThreadSlavePtr mSelfHoldingStartupReferenceUntilPublicationFetchCompletes;

      IConversationThreadForSlaveWeakPtr mBaseThread;
      IAccountForConversationThreadWeakPtr mAccount;

      String mThreadID;
      IPeerLocationPtr mPeerLocation;

      ConversationThreadSlaveStates mCurrentState;

      IConversationThreadDocumentFetcherPtr mFetcher;

      ThreadPtr mHostThread;
      ThreadPtr mSlaveThread;

      IPeerSubscriptionPtr mHostSubscription;
      TimerPtr mHostMessageDeliveryTimer;

      bool mConvertedToHostBecauseOriginalHostLikelyGoneForever;

      typedef String MessageID;
      typedef Time StateChangedTime;
      typedef std::pair<StateChangedTime, IConversationThread::MessageDeliveryStates> DeliveryStatePair;
      typedef std::map<MessageID, DeliveryStatePair> MessageDeliveryStatesMap;
      MessageDeliveryStatesMap mMessageDeliveryStates;

      typedef String CallID;
      typedef std::map<CallID, ICallForConversationThreadPtr> CallHandlers;

      CallHandlers mIncomingCallHandlers;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::IConversationThreadSlaveAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
