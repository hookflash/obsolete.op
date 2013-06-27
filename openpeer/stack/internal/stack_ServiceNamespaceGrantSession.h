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

#include <openpeer/stack/internal/types.h>
#include <openpeer/stack/IBootstrappedNetwork.h>
#include <openpeer/stack/IMessageMonitor.h>
#include <openpeer/stack/IMessageSource.h>
#include <openpeer/stack/IServiceNamespaceGrant.h>

#include <openpeer/stack/IServiceSalt.h>

#include <zsLib/MessageQueueAssociator.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServices
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServices
      {
        typedef IServiceNamespaceGrantSession::SessionStates SessionStates;

        IServiceNamespaceGrantSessionForServices &forServices() {return *this;}
        const IServiceNamespaceGrantSessionForServices &forServices() const {return *this;}

        virtual PUID getID() const = 0;

        virtual String getGrantID() const = 0;

        virtual IServiceNamespaceGrantSessionForServicesWaitPtr obtainWaitToProceed(
                                                                                    IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr waitForWaitUponFailingToObtainDelegate = IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr()
                                                                                    ) = 0;  // returns IServiceNamespaceGrantSessionForServicesWaitPtr() (i.e. NULL) if not obtain to wait at this time

        virtual IServiceNamespaceGrantSessionForServicesQueryPtr query(
                                                                       IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                                                                       const NamespaceGrantChallengeInfo &challengeInfo,
                                                                       const NamespaceInfoMap &namespaces
                                                                       ) = 0;

        virtual bool isNamespaceURLInNamespaceGrantChallengeBundle(
                                                                   ElementPtr bundle,
                                                                   const char *namespaceURL
                                                                   ) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServicesWait
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServicesWait
      {
        virtual PUID getID() const = 0;

        virtual void cancel() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServicesWaitForWaitDelegate
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServicesWaitForWaitDelegate
      {
        virtual void onServiceNamespaceGrantSessionForServicesWaitComplete(IServiceNamespaceGrantSessionPtr session) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServicesQuery
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServicesQuery
      {
        virtual PUID getID() const = 0;

        virtual void cancel() = 0;

        virtual bool isComplete() const = 0;
        virtual ElementPtr getNamespaceGrantChallengeBundle() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServicesQueryDelegate
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServicesQueryDelegate
      {
        virtual void onServiceNamespaceGrantSessionForServicesQueryComplete(
                                                                            IServiceNamespaceGrantSessionForServicesQueryPtr query,
                                                                            ElementPtr namespaceGrantChallengeBundleEl
                                                                            ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionAsyncDelegate
      #pragma mark

      interaction IServiceNamespaceGrantSessionAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceNamespaceGrantSession
      #pragma mark

      class ServiceNamespaceGrantSession : public Noop,
                                           public zsLib::MessageQueueAssociator,
                                           public IServiceNamespaceGrantSession,
                                           public IMessageSource,
                                           public IServiceNamespaceGrantSessionForServices,
                                           public IServiceNamespaceGrantSessionAsyncDelegate,
                                           public IBootstrappedNetworkDelegate
      {
      public:
        friend interaction IServiceNamespaceGrantSessionFactory;
        friend interaction IServiceNamespaceGrantSession;

        typedef IServiceNamespaceGrantSession::SessionStates SessionStates;

        class Wait;
        typedef boost::shared_ptr<Wait> WaitPtr;
        typedef boost::weak_ptr<Wait> WaitWeakPtr;
        friend class Wait;

        class Query;
        typedef boost::shared_ptr<Query> QueryPtr;
        typedef boost::weak_ptr<Query> QueryWeakPtr;
        friend class Query;

        typedef PUID QueryID;
        typedef std::map<QueryID, QueryPtr> QueryMap;

        typedef std::list<DocumentPtr> DocumentList;

        typedef std::list<IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr> WaitingDelegateList;

      protected:
        ServiceNamespaceGrantSession(
                                     IMessageQueuePtr queue,
                                     IServiceNamespaceGrantSessionDelegatePtr delegate,
                                     const char *outerFrameURLUponReload,
                                     const char *grantID
                                     );

        ServiceNamespaceGrantSession(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~ServiceNamespaceGrantSession();

        static ServiceNamespaceGrantSessionPtr convert(IServiceNamespaceGrantSessionPtr session);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => IServiceNamespaceGrantSession
        #pragma mark

        static String toDebugString(IServiceNamespaceGrantSessionPtr session, bool includeCommaPrefix = true);

        static ServiceNamespaceGrantSessionPtr create(
                                                      IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                      const char *outerFrameURLUponReload,
                                                      const char *grantID
                                                      );

        virtual PUID getID() const {return mID;}

        virtual SessionStates getState(
                                       WORD *lastErrorCode,
                                       String *lastErrorReason
                                       ) const;

        virtual String getGrantID() const;

        virtual String getInnerBrowserWindowFrameURL() const;

        virtual void notifyBrowserWindowVisible();
        virtual void notifyBrowserWindowClosed();

        virtual DocumentPtr getNextMessageForInnerBrowerWindowFrame();
        virtual void handleMessageFromInnerBrowserWindowFrame(DocumentPtr unparsedMessage);

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => IMessageSource
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => IServiceNamespaceGrantSessionForServices
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual String getGrantID() const;

        virtual IServiceNamespaceGrantSessionForServicesWaitPtr obtainWaitToProceed(
                                                                                    IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr waitForWaitUponFailingToObtainDelegate = IServiceNamespaceGrantSessionForServicesWaitForWaitDelegatePtr()
                                                                                    );

        virtual IServiceNamespaceGrantSessionForServicesQueryPtr query(
                                                                       IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                                                                       const NamespaceGrantChallengeInfo &challengeInfo,
                                                                       const NamespaceInfoMap &namespaces
                                                                       );

        virtual bool isNamespaceURLInNamespaceGrantChallengeBundle(
                                                                   ElementPtr bundle,
                                                                   const char *namespaceURL
                                                                   ) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => IServiceNamespaceGrantSessionAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => IBootstrappedNetworkDelegate
        #pragma mark

        virtual void onBootstrappedNetworkPreparationCompleted(IBootstrappedNetworkPtr bootstrappedNetwork);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => friend class Query
        #pragma mark

        void notifyQueryGone(PUID queryID);

        // (duplicate) RecursiveLock &getLock() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => friend class Wait
        #pragma mark

        void notifyWaitGone(PUID waitID);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => (internal)
        #pragma mark

        RecursiveLock &getLock() const;

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}

        void step();
        bool stepWaitForServices();
        bool stepPrepareQueries();
        bool stepBootstrapper();
        bool stepLoadGrantWindow();
        bool stepMakeGrantWindowVisible();
        bool stepSendNamespaceGrantStartNotification();
        bool stepWaitForPermission();
        bool stepCloseBrowserWindow();
        bool stepExpiresCheck();

        bool stepRestart();

        void postStep();

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *reason = NULL);
        void sendInnerWindowMessage(MessagePtr message);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession::Wait
        #pragma mark

      public:
        class Wait : public IServiceNamespaceGrantSessionForServicesWait
        {
        public:
          friend class ServiceNamespaceGrantSession;

        protected:
          Wait(ServiceNamespaceGrantSessionPtr outer);

        public:
          ~Wait();

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Wait => IServiceNamespaceGrantSessionForServicesWait
          #pragma mark

          virtual PUID getID() const {return mID;}

          virtual void cancel();

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Wait => friend class ServiceNamespaceGrantSession
          #pragma mark

          static WaitPtr create(ServiceNamespaceGrantSessionPtr outer);

          // (duplicate) virtual PUID getID() const;

        public:
          PUID mID;
          RecursiveLock mLock;
          ServiceNamespaceGrantSessionPtr mOuter;
        };

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession::Query
        #pragma mark

      public:
        class Query : public IServiceNamespaceGrantSessionForServicesQuery
        {
        public:
          friend class ServiceNamespaceGrantSession;

        protected:
          Query(
                ServiceNamespaceGrantSessionPtr outer,
                IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                const NamespaceGrantChallengeInfo &challengeInfo,
                const NamespaceInfoMap &namespaces
                );

        public:
          ~Query();

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Query => IServiceNamespaceGrantSessionForServicesQuery
          #pragma mark

          virtual PUID getID() const {return mID;}

          virtual void cancel();

          virtual bool isComplete() const;
          virtual ElementPtr getNamespaceGrantChallengeBundle() const;

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Query => friend class ServiceNamespaceGrantSession
          #pragma mark

          static QueryPtr create(
                                 ServiceNamespaceGrantSessionPtr outer,
                                 IServiceNamespaceGrantSessionForServicesQueryDelegatePtr delegate,
                                 const NamespaceGrantChallengeInfo &challengeInfo,
                                 const NamespaceInfoMap &namespaces
                                 );

          // (duplicate) virtual PUID getID() const;

          const NamespaceGrantChallengeInfo &getChallengeInfo() const {return mChallengeInfo;}
          const NamespaceInfoMap &getNamespaces() const {return mNamespaces;}

          void notifyComplete(ElementPtr bundleEl);

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Query => (internal)
          #pragma mark

          RecursiveLock &getLock() const;

        public:
          mutable RecursiveLock mBogusLock;
          PUID mID;
          QueryWeakPtr mThisWeak;
          ServiceNamespaceGrantSessionWeakPtr mOuter;

          IServiceNamespaceGrantSessionForServicesQueryDelegatePtr mDelegate;

          NamespaceGrantChallengeInfo mChallengeInfo;
          NamespaceInfoMap mNamespaces;

          ElementPtr mNamespaceGrantChallengeBundleEl;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        ServiceNamespaceGrantSessionWeakPtr mThisWeak;

        IServiceNamespaceGrantSessionDelegatePtr mDelegate;

        BootstrappedNetworkPtr mBootstrappedNetwork;

        IMessageMonitorPtr mNamespaceGrantValidateMonitor;

        SessionStates mCurrentState;

        WORD mLastError;
        String mLastErrorReason;

        String mOuterFrameURLUponReload;

        String mGrantID;

        bool mBrowserWindowReady;
        bool mBrowserWindowVisible;
        bool mBrowserWindowClosed;

        bool mNeedsBrowserWindowVisible;

        bool mNamespaceGrantStartNotificationSent;
        bool mReceivedNamespaceGrantCompleteNotify;

        DocumentList mPendingMessagesToDeliver;

        ULONG mTotalWaits;

        QueryMap mQueriesInProcess;
        QueryMap mPendingQueries;

        WaitingDelegateList mWaitingDelegates;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionFactory
      #pragma mark

      interaction IServiceNamespaceGrantSessionFactory
      {
        static IServiceNamespaceGrantSessionFactory &singleton();

        static ServiceNamespaceGrantSessionPtr create(
                                                      IServiceNamespaceGrantSessionDelegatePtr delegate,
                                                      const char *outerFrameURLUponReload,
                                                      const char *grantID
                                                      );
      };
      
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServiceNamespaceGrantSessionAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServiceNamespaceGrantSessionForServicesWaitForWaitDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::IServiceNamespaceGrantSessionPtr, IServiceNamespaceGrantSessionPtr)
ZS_DECLARE_PROXY_METHOD_1(onServiceNamespaceGrantSessionForServicesWaitComplete, IServiceNamespaceGrantSessionPtr)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServiceNamespaceGrantSessionForServicesQueryDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IServiceNamespaceGrantSessionForServicesQueryPtr, IServiceNamespaceGrantSessionForServicesQueryPtr)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::XML::ElementPtr, ElementPtr)
ZS_DECLARE_PROXY_METHOD_2(onServiceNamespaceGrantSessionForServicesQueryComplete, IServiceNamespaceGrantSessionForServicesQueryPtr, ElementPtr)
ZS_DECLARE_PROXY_END()
