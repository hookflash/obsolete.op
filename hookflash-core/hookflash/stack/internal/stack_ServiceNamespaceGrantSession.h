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

#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IMessageMonitor.h>
#include <hookflash/stack/IMessageSource.h>
#include <hookflash/stack/IServiceNamespaceGrant.h>
#include <hookflash/stack/message/namespace-grant/NamespaceGrantValidateResult.h>

#include <hookflash/stack/IServiceSalt.h>

#include <zsLib/MessageQueueAssociator.h>

#include <list>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using message::namespace_grant::NamespaceGrantValidateResult;
      using message::namespace_grant::NamespaceGrantValidateResultPtr;

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

        virtual SessionStates getState(
                                       WORD *lastErrorCode,
                                       String *lastErrorReason
                                       ) const = 0;

        virtual IServiceNamespaceGrantSessionForServicesSubscriptionPtr subscribe(
                                                                                  IServiceNamespaceGrantSessionForServicesDelegatePtr delegate
                                                                                  ) = 0;

        virtual void grantNamespaces(const NamespaceInfoMap &namespaces) = 0;
        virtual bool isNamespaceGranted(const char *namespaceURL) const = 0;

        virtual GrantInfo getGrantInfo() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServicesDelegate
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServicesDelegate
      {
        typedef IServiceNamespaceGrantSession::SessionStates SessionStates;

        virtual void onServiceNamespaceGrantSessionStateChanged(
                                                                ServiceNamespaceGrantSessionPtr session,
                                                                SessionStates state
                                                                ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IServiceNamespaceGrantSessionForServicesSubscription
      #pragma mark

      interaction IServiceNamespaceGrantSessionForServicesSubscription
      {
        virtual PUID getID() const = 0;

        virtual void cancel() = 0;
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
                                           public IBootstrappedNetworkDelegate,
                                           public IMessageMonitorResultDelegate<NamespaceGrantValidateResult>
      {
      public:
        friend interaction IServiceNamespaceGrantSessionFactory;
        friend interaction IServiceNamespaceGrantSession;

        typedef IServiceNamespaceGrantSession::SessionStates SessionStates;

        class Subscription;
        typedef boost::shared_ptr<Subscription> SubscriptionPtr;
        typedef boost::weak_ptr<Subscription> SubscriptionWeakPtr;
        friend class Subscription;

        typedef PUID SubscriptionID;
        typedef std::map<SubscriptionID, SubscriptionWeakPtr> SubscriptionMap;

        typedef std::list<DocumentPtr> DocumentList;

      protected:
        ServiceNamespaceGrantSession(
                                     IMessageQueuePtr queue,
                                     BootstrappedNetworkPtr network,
                                     IServiceNamespaceGrantSessionDelegatePtr delegate,
                                     const char *outerFrameURLUponReload,
                                     const char *grantID,
                                     const char *grantSecret
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
                                                      IServiceNamespaceGrantPtr serviceNamespaceGrant,
                                                      const char *outerFrameURLUponReload,
                                                      const char *grantID,
                                                      const char *grantSecret
                                                      );

        virtual PUID getID() const {return mID;}

        virtual IServiceNamespaceGrantPtr getService() const;

        virtual SessionStates getState(
                                       WORD *lastErrorCode,
                                       String *lastErrorReason
                                       ) const;

        virtual String getGrantID() const;
        virtual String getGrantSecret() const;

        virtual void notifyAssocaitedToAllServicesComplete();

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

        // (duplicate) virtual SessionStates getState(
        //                                            WORD *lastErrorCode,
        //                                            String *lastErrorReason
        //                                            ) const;

        virtual IServiceNamespaceGrantSessionForServicesSubscriptionPtr subscribe(
                                                                                  IServiceNamespaceGrantSessionForServicesDelegatePtr delegate
                                                                                  );

        virtual void grantNamespaces(const NamespaceInfoMap &namespaceURL);
        virtual bool isNamespaceGranted(const char *namespaceURL) const;

        virtual GrantInfo getGrantInfo() const;

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

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => IMessageMonitorResultDelegate<NamespaceGrantValidateResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        NamespaceGrantValidateResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             NamespaceGrantValidateResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession => friend class Subscription
        #pragma mark

        void notifySubscriptionGone(PUID subscriptionID);

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
        bool stepBootstrapper();
        bool stepSendInitialValidate();
        bool stepCheckAllNamespaces();
        bool stepLoadGrantWindow();
        bool stepMakeGrantWindowVisible();
        bool stepSendNamespaceGrantStartNotification();
        bool stepWaitForPermission();
        bool stepCloseBrowserWindow();
        bool stepExpiresCheck();

        void postStep();

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *reason = NULL);
        void sendInnerWindowMessage(MessagePtr message);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceNamespaceGrantSession::Subscription
        #pragma mark

      public:
        class Subscription : public IServiceNamespaceGrantSessionForServicesSubscription
        {
        public:
          friend class ServiceNamespaceGrantSession;

          typedef ServiceNamespaceGrantSession::SessionStates SessionStates;

        protected:
          Subscription(
                       ServiceNamespaceGrantSessionPtr outer,
                       IServiceNamespaceGrantSessionForServicesDelegatePtr delegate
                       );

        public:
          ~Subscription();

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Subscription => IServiceNamespaceGrantSessionForServices
          #pragma mark

          virtual PUID getID() const {return mID;}

          virtual void cancel();

        protected:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark ServiceNamespaceGrantSession::Subscription => friend class ServiceNamespaceGrantSession
          #pragma mark

          static SubscriptionPtr create(
                                        ServiceNamespaceGrantSessionPtr outer,
                                        IServiceNamespaceGrantSessionForServicesDelegatePtr delegate
                                        );

          // (duplicate) virtual PUID getID() const;

          void notifyStateChanged(SessionStates state);

        public:
          PUID mID;
          ServiceNamespaceGrantSessionPtr mOuter;

          IServiceNamespaceGrantSessionForServicesDelegatePtr mDelegate;
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

        GrantInfo mGrantInfo;

        bool mRedoGrantBrowserProcessIfNeeded;

        bool mNotifiedAssociatedToAllServicesComplete;

        bool mBrowserWindowReady;
        bool mBrowserWindowVisible;
        bool mBrowserWindowClosed;

        bool mNeedsBrowserWindowVisible;

        ULONG mTotalNamesapceValidationsIssued;
        Time mLastNamespaceValidateIssued;
        bool mNamespaceGrantStartNotificationSent;
        bool mReceivedNamespaceGrantCompleteNotify;

        DocumentList mPendingMessagesToDeliver;

        NamespaceInfoMap mNamespacesToGrant;
        NamespaceInfoMap mNamespacesGranted;
        bool mHasGrantedAllRequiredNamespaces;

        SubscriptionMap mSubscriptions;
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
                                                      IServiceNamespaceGrantPtr serviceNamespaceGrant,
                                                      const char *outerFrameURLUponReload,
                                                      const char *grantID,
                                                      const char *grantSecret
                                                      );
      };
      
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServiceNamespaceGrantSessionAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IServiceNamespaceGrantSessionForServicesDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::ServiceNamespaceGrantSessionPtr, ServiceNamespaceGrantSessionPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IServiceNamespaceGrantSessionForServicesDelegate::SessionStates, SessionStates)
ZS_DECLARE_PROXY_METHOD_2(onServiceNamespaceGrantSessionStateChanged, ServiceNamespaceGrantSessionPtr, SessionStates)
ZS_DECLARE_PROXY_END()
